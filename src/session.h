/* Copyright Hengfeng Lang. Inspiredd by libuv link: http://www.libuv.org
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), toi
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __session_h__
#define __session_h__

#include <iostream>
#include <tuple>
#include "defer.h"
#include "column.h"
#include "mysqlopr.h"
#include "engine.h"

#define _DISTINCT(x) x.name = "distinct "#x
#define _SUM(x, y) x.name = "sum(" #y ")"
#define _COUNT(x) x.name = "count(1)"
#define _FIELD(x) x.name = #x
#define _DEFINE(T, args...) \
    T() {\
        Session::Define(args);\
    }
#define _CONSTRUCT(N, args...) \
    void Construct(MYSQL_BIND *bind, integral_constant<int, N>) {\
        uint32_t location = 0; \
        Session::Construct(bind, location, args);\
    }\
    string GetField(integral_constant<int, N>) {\
        ostringstream   os;\
        Session::LinkName(os, args);\
        return os.str();\
    }
#define _AND(args...) Session::And(args)
#define _OR(args...) Session::Or(args)

namespace MySqlOrm {
class Session {
public:
    Session() {}
    ~Session() {}

    void Clear() {
        m_params.clear();
        m_sql.str("");
    }
    string String() {
        return m_sql.str();
    }
    void Bind(Engine * engine) {
        m_engine = engine;
    }
    int Execute() {
        int ret = DoSql([&](MYSQL_STMT* stmt) {
            return MySqlOpr::ExecuteSql(stmt, m_insertId);
        });
        return ret;
    }    
    template<typename T, int N = 0>
    int GetAll(vector<T> & result) {
        int ret = DoSql([&result](MYSQL_STMT* stmt) {
            return MySqlOpr::QuerySql<T, N>(stmt, result);
        });
        return ret;
    }
    uint64_t InsertId() {
        return m_insertId;
    }
    unsigned int Errno() {
        return m_errno;
    }
    const char * Errstr() {
        return m_errstr.c_str();
    }
    Session & Table(const string & table) {
        m_table = table;
        return *this;
    }
    Session & Limit(uint32_t cnt) {
        m_sql << " limit " << cnt;
        return *this;
    }
    template<typename... Args>
    Session & Orderby(Args &&...  args) {
        m_sql << " order by ";
        LinkName(m_sql, args...);
        return *this;
    }
    template<typename... Args>
    Session & OrderbyDesc(Args &&...  args) {
        Orderby(args...);
        m_sql << " desc";
        return *this;
    }
    template<typename... Args>
    Session & Groupby(Args &&...  args) {
        m_sql << " group by ";
        LinkName(m_sql, args...);
        return *this;
    }
    template<typename... Args>
    Session & Filter(Args &&...  args) {
        m_sql << " where ";
        _Filter(" and ", args...);
        return *this;
    }
    template<typename... Args>
    static tuple<string, vector<MYSQL_BIND>> And(Args &&...  args) {
        ostringstream       osStr;
        osStr << "(";
        vector<MYSQL_BIND>  params;
        Link(osStr, params, " and ", args...);
        return make_tuple(osStr.str(), params);
    }
    template<typename... Args>
    static tuple<string, vector<MYSQL_BIND>> Or(Args &&...  args) {
        ostringstream       osStr;
        osStr << "(";
        vector<MYSQL_BIND>  params;
        Link(osStr, params, " or ", args...);
        return make_tuple(osStr.str(), params);
    }
    template<typename T, int N = 0>
    Session & Query() {
        Clear();
        m_sql << "select " << T().GetField(integral_constant<int, N>()) << " from " << m_table;
        return *this;
    }
    template<typename... Args>
    Session & Insert(Args &&... args) {
        Clear();
        m_sql << "insert into " << m_table << "(";
        _Insert(args...);
        return *this;
    }
    template<typename... Args>
    Session & Replace(Args &&... args) {
        Clear();
        m_sql << "replace into " << m_table << "(";
        _Insert(args...);
        return *this;
    }
    template<typename... Args>
    Session & Values(Args &&... args) {
        m_sql << " values(";
        _Values(args...);
        return *this;
    }
    template<typename... Args>
    Session & operator ()(Args &&... args) {
        m_sql << ",(";
        _Values(args...);
        return *this;
    }
    template<typename... Args>
    Session & Update(Args &&... args) {
        Clear();
        m_sql << "update " << m_table << " set ";
        _Update(args...);
        return *this;
    }
    template<typename... Args>
    Session & OnDupKey(Args &&... args) {
        m_sql << " on duplicate key update ";
        _OnDupKey(args...);
        return *this;
    }
    Session & Text(const string & sqlstr) {
        Clear();
        m_sql << sqlstr;
        return *this;
    }
    template<typename... Args>
    Session & Text(const string & sqlstr, Args&&... args) {
        Clear();
        m_sql << sqlstr;
        _Text(args...);
        return *this;
    }
    template<typename T, int L>
    static void Construct(MYSQL_BIND *bind, uint32_t & location, Column<T, L> & field) {
        MySqlOpr::Bind<T, L>(bind[location++], field.val);
    }
    template<typename T, int L, typename... Args>
    static void Construct(MYSQL_BIND * bind, uint32_t & location, Column<T, L> & field, Args&&... args) {
        MySqlOpr::Bind<T, L>(bind[location++], field.val);
        Construct(bind, location, args...);
    }
    
    void Print() {
        cout << "SQL: " << m_sql.str() << endl;
        cout << "VALUE: (";
        bool   first = true;
        for (auto param : m_params) {
            if (!first) {
                cout << ",";
            }            
            first = false;
            if (param.buffer_type == MYSQL_TYPE_STRING) {
                cout << (char *)param.buffer ;
            } else if (param.buffer_type  == MYSQL_TYPE_SHORT) {
                cout << *(int16_t *)param.buffer;
            } else if (param.buffer_type  == MYSQL_TYPE_LONG) {
                cout << *(int32_t *)param.buffer;
            } else if (param.buffer_type  == MYSQL_TYPE_LONGLONG) {
                cout << *(int64_t *)param.buffer;
            } else if (param.buffer_type  == MYSQL_TYPE_FLOAT) {
                cout << *(float *)param.buffer;
            } else if (param.buffer_type  == MYSQL_TYPE_DOUBLE) {
                cout << *(double *)param.buffer;
            }
        }
        cout << ")" << endl;;
    }
    template<typename... Args>
    static void Define(Args&&... args) {}

    template<typename T, int L>
    static void LinkName(ostringstream& os, Column<T, L>& field) {
        os << field.name;
    }
    template<typename T, int L, typename... Args>
    static void LinkName(ostringstream& os, Column<T, L>& field, Args&&... args) {
        os << field.name << " ,";
        LinkName(os, args...);
    }

private:
    template<typename T>
    inline void _Filter(const string & link, const tuple<string, T>& expr) {
        m_sql << get<0>(expr);
        Append(m_params, get<1>(expr));
    }
    template<typename T, typename... Args>
    inline void _Filter(const string & link, const tuple<string, T>& expr, Args &&... args) {
        m_sql << get<0>(expr) << link;
        Append(m_params, get<1>(expr));
        _Filter(link, args...);
    }
    template<typename T, int L>
    inline void _Insert(Column<T, L>& field) {
        m_sql << field.name << ")";
    }
    template<typename T, int L, typename... Args>
    inline void _Insert(Column<T, L>& field, Args &&... args) {
        m_sql << field.name << ",";
        _Insert(args...);
    }
    template<typename T>
    inline void _Values(T && value) {
        m_sql << "?)";
        Append(m_params, value);
    }
    template<typename T, typename... Args>
    inline void _Values(T && value, Args &&... args) {
        m_sql << "?,";
        Append(m_params, value);
        _Values(args...);
    }
    template<typename T, int L>
    inline void _Update(Column<T, L>& field) {
        m_sql << field.name << " = ?";
        Append(m_params, field.val);
    }
    template<typename T, int L, typename... Args>
    inline void _Update(Column<T, L>& field, Args &&... args) {
        m_sql << field.name << " = ?,";
        Append(m_params, field.val);
        _Update(args...);
    }
    inline void _OnDupKey(const string & field) {
        m_sql << field;
    }
    template<typename... Args>
    inline void _OnDupKey(const string & field, Args &&... args) {
        m_sql << field << ",";
        _OnDupKey(args...);
    }
    template<typename T>
    Session & _Text(T && value) {
        Append(m_params, value);
        return *this;
    }
    template<typename T, typename... Args>
    Session & _Text(T && value, Args&&... args) {
        Append(m_params, value);
        _Text(args...);
        return *this;
    }
    
    inline int DoSql(function<int(MYSQL_STMT*)> sqlcmd) {
        if (m_engine == nullptr) {
            return -1;
        }
        MyHdl    hdl = m_engine->GetHdl();
        Defer   guard([&]{
            m_engine->RetHdl(hdl);
        });
        Defer   guardErr([&]{
            m_errno = mysql_stmt_errno(hdl.stmt);
            m_errstr = mysql_stmt_error(hdl.stmt);
        });        
        
        if (hdl.conn == nullptr || hdl.stmt == nullptr) {
            return -1;
        }        
        if (MySqlOpr::PrepareSql(hdl.stmt, m_sql.str(), m_params) != 0) {
            return -1;
        }
        if (sqlcmd && sqlcmd(hdl.stmt) < 0) {
            return -1;
        }
        guardErr.Dismiss();
        return 0;
    }
    template<typename T>
    static inline typename enable_if<is_pointer<T>::value>::type Append(vector<MYSQL_BIND>& params, T value) {
        MYSQL_BIND      param;
        memset(&param, 0, sizeof(param));
        MySqlOpr::Bind(param, value);
        params.emplace_back(param);
    }
    template<typename T>
    static inline typename enable_if<is_arithmetic<T>::value>::type Append(vector<MYSQL_BIND>& params, T & value) {
        MYSQL_BIND      param;
        memset(&param, 0, sizeof(param));
        MySqlOpr::Bind(param, value);
        params.emplace_back(param);
    }
    static inline void Append(vector<MYSQL_BIND>& params, string & value) {
        MYSQL_BIND      param;
        memset(&param, 0, sizeof(param));
        param.buffer_type = MYSQL_TYPE_STRING;
        param.buffer = (void *)value.c_str();
        param.buffer_length = value.size();
        params.emplace_back(param);
    }
    static inline void Append(vector<MYSQL_BIND>& params, const vector<MYSQL_BIND> & param) {
        for (auto v : param) {
             params.emplace_back(v);
        }
    }
    template<typename T>
    static inline void Link(ostringstream & osStr, vector<MYSQL_BIND>& params, 
                            const string & op, const tuple<string, T>& expr) {
        osStr << get<0>(expr) << ")";
        Append(params, get<1>(expr));
    }
    template<typename T, typename... Args>
    static inline void Link(ostringstream & osStr, vector<MYSQL_BIND>& params, 
                            const string & op, const tuple<string, T>& expr, Args &&... args) {
        osStr << get<0>(expr) << op;
        Append(params, get<1>(expr));
        return Link(osStr, params, op, args...);
    }    
    
    string              m_table;
    ostringstream       m_sql;
    vector<MYSQL_BIND>  m_params;
    Engine              *m_engine;
    uint64_t            m_insertId;
    unsigned int        m_errno;
    string              m_errstr;
};
};

#endif

