//--------------------------------------------------------------------------------
// Copyright (c) 2017-2019, sanko-shoko. All rights reserved.
//--------------------------------------------------------------------------------

#ifndef __SPIO_H__
#define __SPIO_H__

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<vector>
#include<map>

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#pragma warning(disable:4819)
#pragma warning(disable:4101)
#endif


namespace spio {

#ifndef SPIO_FUNC
#define SPIO_FUNC static
#endif

#ifndef SPIO_USE_PRINT
#define SPIO_USE_PRINT 0
#endif

#if SPIO_USE_PRINT
#define SPIO_PRINTF(...) ::printf(__VA_ARGS__);
#else
#define SPIO_PRINTF(...) if(0){ ::printf(__VA_ARGS__); }
#endif


    //--------------------------------------------------------------------------------
    // node type
    //--------------------------------------------------------------------------------

    enum NODE_TYPE {
        NON_NODE = 0,
        TXT_NODE = 1,
        BIN_NODE = 2,
        OBJ_NODE = 3,
    };


    //--------------------------------------------------------------------------------
    // writer
    //--------------------------------------------------------------------------------

    class Writer {

    private:
        // file path
        std::string m_path;

        // data buffer
        std::vector<unsigned char> m_buff;

        // nest stack
        std::vector<int> m_stack;

    public:

        Writer() {
        }
        Writer(const std::string &path) {
            init(path);
        }

        void init(const std::string &path) {
            m_path = path;

            m_buff.clear();
            m_stack.clear();
        }


        //--------------------------------------------------------------------------------
        // text
        //--------------------------------------------------------------------------------

        void addTxt(const std::string &name, const std::string &text) {
            _addName(m_buff, name, TXT_NODE);

            _addTxt(m_buff, text);
            _addTxt(m_buff, "\n");
        }

        template<typename TYPE>
        void addTxt(const std::string &name, const std::string &format, const TYPE &data) {
            _addName(m_buff, name, TXT_NODE);

            _addTxt(m_buff, _string(format.c_str(), data));
            _addTxt(m_buff, "\n");
        }

        template<typename TYPE>
        void addTxt(const std::string &name, const std::string &format, const TYPE *data, const int size) {
            _addName(m_buff, name, TXT_NODE);

            for (int i = 0; i < size; i++) {
                _addTxt(m_buff, _string(format.c_str(), data[i]) + ((i < size - 1) ? "," : ""));
            }
            _addTxt(m_buff, "\n");
        }


        //--------------------------------------------------------------------------------
        // binary
        //--------------------------------------------------------------------------------

        void addBin(const std::string &name, const void *data, const int size) {
            _addName(m_buff, name, BIN_NODE);

            _addTxt(m_buff, _string("%d ", size));
            _addBin(m_buff, data, size);
            _addTxt(m_buff, "\n");
        }

        template<typename TYPE>
        void addBin(const std::string &name, const TYPE &data) {
            _addName(m_buff, name, BIN_NODE);

            _addTxt(m_buff, _string("%d,", sizeof(TYPE)));
            _addBin(m_buff, &data, sizeof(TYPE));
            _addTxt(m_buff, "\n");
        }


        //--------------------------------------------------------------------------------
        // nest
        //--------------------------------------------------------------------------------

        void nest(const std::string &name) {
            _addName(m_buff, name, OBJ_NODE);
            _addTxt(m_buff, "\n");

            m_stack.push_back((int)m_buff.size() - 1);
        }

        void unnest() {
            const int size = (int)(m_buff.size() - m_stack.back());

            const std::string str = _string("%d", size - 1);
            const std::vector<unsigned char> temp(&str[0], &str[str.size()]);

            _insert(m_buff, -size, &temp[0], (int)temp.size());

            m_stack.pop_back();
        }


        //--------------------------------------------------------------------------------
        // object
        //--------------------------------------------------------------------------------

        template<typename TYPE>
        void addObj(const std::string &name, const TYPE &data) {
            nest(name);
            _addObj(*this, data);
            unnest();
        }


        //--------------------------------------------------------------------------------
        // file
        //--------------------------------------------------------------------------------

        bool flush() {
            bool ret = false;

            FILE *fp = fopen(m_path.c_str(), "wb");
            if (fp != NULL) {
                fwrite(&m_buff[0], 1, m_buff.size(), fp);
                fclose(fp);
                ret = true;
            }
            return ret;
        }


        //--------------------------------------------------------------------------------
        // util
        //--------------------------------------------------------------------------------

        void print() {
            for (int i = 0; i < m_buff.size(); i++) {
                const char *c = (const char *)&m_buff[i];
                SPIO_PRINTF("%c", *c);
            }
        }


    private:

        //--------------------------------------------------------------------------------
        // internal
        //--------------------------------------------------------------------------------

        template<typename TYPE>
        std::string _string(const std::string &format, const TYPE &data) {
            char str[256];
            sprintf(str, format.c_str(), data);
            return std::string(str);
        }

        void _insert(std::vector<unsigned char> &buff, const int offset, const void *data, const int size) {
            const unsigned char *d = (const unsigned char*)data;
            const std::vector<unsigned char> temp(&d[0], &d[size]);
            buff.insert(buff.end() + offset, temp.begin(), temp.end());
        }

        template<typename TYPE>
        void _addBin(std::vector<unsigned char> &buff, const TYPE &data) {
            _insert(buff, 0, &data, sizeof(TYPE));
        }

        void _addBin(std::vector<unsigned char> &buff, const void *data, const int size) {
            _insert(buff, 0, data, size);
        }

        void _addTxt(std::vector<unsigned char> &buff, const std::string &text) {
            _insert(buff, 0, text.c_str(), (int)text.size());
        }

        void _addName(std::vector<unsigned char> &buff, const std::string &name, const NODE_TYPE &type) {
            for (int i = 0; i < m_stack.size(); i++) {
                _addTxt(buff, " ");
            }
            switch (type) {
            case TXT_NODE: _addTxt(buff, "(" + name + ")"); break;
            case BIN_NODE: _addTxt(buff, "{" + name + "}"); break;
            case OBJ_NODE: _addTxt(buff, "[" + name + "]"); break;
            }
        }

    };


    class _Nest {

    private:
        Writer *writer;

    public:
        _Nest(Writer &writer, const std::string &name) {
            this->writer = &writer;
            nest(name);
        }
        ~_Nest() {
            unnest();
        }
        void nest(const std::string &name) {
            writer->nest(name);
        }
        void unnest() {
            writer->unnest();
        }
    };

#define SPIO_NEST(WRITER, NAME) spio::_Nest _nest(WRITER, NAME);


    //--------------------------------------------------------------------------------
    // node
    //--------------------------------------------------------------------------------
 
    class Node {
        friend class Reader;

    private:

        // node name
        std::string m_name;

        // node type
        NODE_TYPE m_type;

        // data size
        int m_size;

        // data pointer
        void *m_ptr;

        // child nodes
        std::vector<Node*> m_cnodes;

    public:

        Node() {
            m_type = NON_NODE;
            m_size = 0;
            m_ptr = NULL;
        }

        Node(const Node &node) {
            *this = node;
        }

        const Node& operator = (const Node &node) {
            m_name = node.m_name;
            m_type = node.m_type;
            m_size = node.m_size;
            m_ptr = node.m_ptr;
            m_cnodes = node.m_cnodes;
            return *this;
        }

        
        //--------------------------------------------------------------------------------
        // node
        //--------------------------------------------------------------------------------

        const std::vector<Node*> getCNodes() const {
            return m_cnodes;
        }
        
        const std::vector<Node*> getCNodes(const std::string &name) const {
            std::vector<Node*> ret;
            for (int i = 0; i < m_cnodes.size(); i++) {
                if (m_cnodes[i]->m_name == name) {
                    ret.push_back(m_cnodes[i]);
                }
            }
            return ret;
        }

        const Node* getCNode(const int p = 0) const {
            Node* ret = NULL;
            if (p < (int)m_cnodes.size()) {
                ret = m_cnodes[p];
            }
            return ret;
        }

        const Node* getCNode(const std::string &name, const int p = 0) const {
            Node* ret = NULL;
            const std::vector<Node*> list = getCNodes(name);
            if (p < (int)list.size()) {
                ret = list[p];
            }
            return ret;
        }


        //--------------------------------------------------------------------------------
        // data
        //--------------------------------------------------------------------------------
        
        const std::string getTxt(const int p = 0) const {
            std::string ret;
            if (_cnvTxt(ret, p) == false) {
                throw "spio:convert error\n";
            }
            return ret;
        }

        template<typename TYPE>
        const TYPE getBin(const int p = 0) const {
            TYPE ret;
            if (_cnvBin<TYPE>(ret, p) == false) {
                throw "spio:convert error\n";
            }
            return ret;
        }

        bool _cnvTxt(std::string &dst, int p = 0) const {
            bool ret = false;
            if (m_type != TXT_NODE) return ret;

            const std::vector<std::string> list = _divTxt(m_ptr, m_size);
            if (p < list.size()) {
                dst = list[p];
                ret = true;
            }
            return ret;
        }

        template<typename TYPE>
        bool _cnvBin(TYPE &dst, const int p = 0) const {
            bool ret = false;
            if (m_type != BIN_NODE) return ret;

            const std::vector<TYPE> list = _divBin<TYPE>(m_ptr, m_size);
            if (p < list.size()) {
                dst = list[p];
                ret = true;
            }
            return ret;
        }


        //--------------------------------------------------------------------------------
        // util
        //--------------------------------------------------------------------------------

        const int elms() const {
            int ret = 0;
            switch (m_type) {
            case TXT_NODE: ret = (int)_divTxt(m_ptr, m_size).size(); break;
            case BIN_NODE: ret = m_size; break;
            case OBJ_NODE: ret = (int)m_cnodes.size(); break;
            default: break;
            }
            return ret;
        }

        const NODE_TYPE& type() const {
            return m_type;
        }

        const std::string& name() const {
            return m_name;
        }

    private:

        std::vector<std::string> _divTxt(const void *ptr, const int size) const {
            std::vector<std::string> ret;
            if (size == 0) return ret;

            const char *p = (const char*)ptr;
            std::string src(&p[0], &p[size]);

            char tok = ',';
            int s = 0;
            int e = (int)src.find_first_of(tok);
            e = (e > 0) ? e : src.size();

            while (s < src.size()) {
                std::string sub(src, s, e - s);

                ret.push_back(sub);

                s = e + 1;
                e = (int)src.find_first_of(tok, s);

                if (e == std::string::npos) {
                    e = (int)src.size();
                }
            }
            return ret;
        }

        template<typename TYPE>
        std::vector<TYPE> _divBin(const void *ptr, const int size) const {
            std::vector<TYPE> ret;

            const TYPE *p = (const TYPE*)ptr;
            for (int i = 0; i < size / sizeof(TYPE); i++) {
                ret.push_back(p[i]);
            }
            return ret;
        }

    };


    //--------------------------------------------------------------------------------
    // reader
    //--------------------------------------------------------------------------------
 
    class Reader {

    private:
        // file path
        std::string m_path;
        
        // data buffer
        std::vector<unsigned char> m_buff;

        std::vector<Node> m_cnodes;

    public:

        Reader() {
        }

        Reader(const std::string &path) {
            init(path);
        }

        void init(const std::string &path) {
            m_path = path;
            m_buff.clear();
            m_cnodes.clear();
        }


        //--------------------------------------------------------------------------------
        // file
        //--------------------------------------------------------------------------------

        bool parse(){
            bool ret = false;

            FILE *fp = fopen(m_path.c_str(), "rb");
            if (fp != NULL) {
                {
                    size_t size = 0;
                    fseek(fp, 0L, SEEK_END);
                    size = (size_t)ftell(fp);
                    fseek(fp, 0L, SEEK_SET);
                 
                    m_buff.resize(size);
                }

                fread(&m_buff[0], 1, m_buff.size(), fp);
                fclose(fp);
            }

            if (m_buff.size() > 0) {
                ret = _parse();
            }

            return ret;
        }

        //--------------------------------------------------------------------------------
        // util
        //--------------------------------------------------------------------------------

        Node* root() {
            return (m_cnodes.size() > 0) ? &m_cnodes[0] : NULL;
        }

        void print() {
            for (int i = 0; i < m_buff.size(); i++) {
                const char *c = (const char *)&m_buff[i];
                SPIO_PRINTF("%c", *c);
            }
        }

    private:

        //--------------------------------------------------------------------------------
        // internal
        //--------------------------------------------------------------------------------

        unsigned char& _getv(const int i) {
            if (i < m_buff.size()) {
                return m_buff[i];
            }
            else {
                throw "spio:format error\n";
            }
        }

        bool _parse() {
            std::vector<int> indent;
            indent.push_back(-1);

            m_cnodes.clear();
            m_cnodes.push_back(Node());

            try {
                for (int i = 0; i < (int)m_buff.size();) {
                    Node node;

                    int pos = i;
                    {
                        for (; ; pos++) {
                            if (_getv(pos) == '(' || _getv(pos) == '{' || _getv(pos) == '[') break;
                        }
                        indent.push_back(pos - i);

                        switch (_getv(pos))
                        {
                        case '(': node.m_type = TXT_NODE; break;
                        case '{': node.m_type = BIN_NODE; break;
                        case '[': node.m_type = OBJ_NODE; break;
                        default: break;
                        }
                        pos++;
                    }

                    {
                        for (; ; pos++) {
                            if (_getv(pos) == ')' || _getv(pos) == '}' || _getv(pos) == ']') break;
                            node.m_name.push_back((char)_getv(pos));
                        }
                        pos++;
                    }
                    {
                        const int spos = pos;

                        switch (node.m_type) {
                        case TXT_NODE:
                        {
                            // data step
                            for (; ; pos++) {
                                if (_getv(pos) == '\n') break;
                            }
                            pos++;

                            node.m_size = pos - spos - 1;
                            node.m_ptr = &_getv(spos);
                            break;
                        }
                        case BIN_NODE:
                        {
                            // size step
                            for (; ; pos++) {
                                if (_getv(pos) == ',') break;
                            }
                            pos++;

                            node.m_ptr = &_getv(pos);
                            node.m_size = atoi(std::string(&_getv(spos), &_getv(pos - 1)).c_str());
                            
                            // data step
                            pos += node.m_size + 1;
                            break;
                        }
                        case OBJ_NODE:
                        {
                            // size step
                            for (; ; pos++) {
                                if (_getv(pos) == '\n') break;
                            }
                            pos++;

                            node.m_ptr = &_getv(pos);
                            node.m_size = atoi(std::string(&_getv(spos), &_getv(pos - 1)).c_str());
                            break;
                        }
                        }
                    }

                    m_cnodes.push_back(node);
                    i += (pos - i);
                }
            }
            catch (char *str) {
                SPIO_PRINTF(str);
                throw str;
            }

            std::vector<Node*> ptrs;
            for (int i = 1; i < (int)m_cnodes.size(); i++) {
                Node &node = m_cnodes[i];

                const int crnt = indent[i];
                const int prev = indent[i - 1];
                if (crnt > prev) {
                    ptrs.push_back(&m_cnodes[i - 1]);
                }
                else if(crnt < prev){
                    ptrs.pop_back();
                }
                Node *base = ptrs[crnt];

                base->m_cnodes.push_back(&node);
            }

            return true;
        }

    };
}

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif