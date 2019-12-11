#define SPIO_USE_PRINT 1
#include "spio.h"


struct Data {
    int a;
    double b;
};

void _addObj(spio::Writer &writer, const Data &data) {
    writer.addTxt("a", "%d", data.a);
    writer.addTxt("b", "%.1lf", data.b);
}

int main(){

    {
        spio::Writer writer("test.sp");

        {
            const int a = 10;
            const double b = 10.1;

            writer.addTxt("a", "%d", a);
            writer.addTxt("b", "%.1lf", b);
        }
        {
            const int c[] = { 1, 2, 3 };
            writer.addTxt("c", "%d", c, 3);
        }
        {
            const int d = 100;
            writer.addBin("d", d);
        }
        {
            SPIO_NEST(writer, "data");

            Data data;
            data.a = 10;
            data.b = 10.1;

            writer.addTxt("a", "%d", data.a);
            writer.addTxt("b", "%.1lf", data.b);
        }
        {
            Data data;
            data.a = 10;
            data.b = 10.1;
            writer.addObj("data", data);
        }

        writer.print();
        writer.flush();
    }
    
    printf("\n");
    {
        spio::Reader reader("test.sp");

        reader.parse();
        //reader.print();
        const spio::Node* root = reader.root();

        try {
            auto printTxt = [&](const spio::Node* node) {
                printf("%s ", node->name().c_str());
                const int size = node->elms();
                for (int p = 0; p < size; p++) {
                    printf("%s%c", node->getTxt(p).c_str(), (p == (size - 1) ? '\n' : ','));
                }
            };
            auto printInt = [&](const spio::Node* node) {
                printf("%s ", node->name().c_str());
                const int size = node->elms() / sizeof(int);
                for (int p = 0; p < size; p++) {
                    printf("%d%c", node->getBin<int>(p), (p == (size - 1) ? '\n' : ','));
                }
            };
            auto printData = [&](const spio::Node* node) {
                printf("%s\n", node->name().c_str());
                printTxt(node->getCNode("a"));
                printTxt(node->getCNode("b"));
            };

            printTxt(reader.root()->getCNode("a"));
            printTxt(reader.root()->getCNode("b"));
            printTxt(reader.root()->getCNode("c"));
            printInt(reader.root()->getCNode("d"));
            printData(reader.root()->getCNode("data", 0));
            printData(reader.root()->getCNode("data", 1));
        }
        catch (char *str) {
            printf(str);
        }
    }
    return 0;
}