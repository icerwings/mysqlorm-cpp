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

#ifndef __mysqlopr_h__
#define __mysqlopr_h__

#include <stdint.h>
#include <string.h>
#include <vector>
#include "defer.h"
#include "mysql.h"
using namespace std;
using namespace Libmv;

namespace MySqlOrm {
class MySqlOpr {
public:
    template<typename T>
    static inline enum_field_types MyType(T *) {
        if (is_same<int16_t, typename decay<T>::type>::value
            || is_same<uint16_t, typename decay<T>::type>::value) {
            return MYSQL_TYPE_SHORT;
        } else if (is_same<int32_t, typename decay<T>::type>::value
            || is_same<uint32_t, typename decay<T>::type>::value) {
            return MYSQL_TYPE_LONG;
        } else if (is_same<int64_t, typename decay<T>::type>::value
            || is_same<uint64_t, typename decay<T>::type>::value) {
            return MYSQL_TYPE_LONGLONG;
        } else if (is_same<float, typename decay<T>::type>::value) {
            return MYSQL_TYPE_FLOAT;
        } else if (is_same<double, typename decay<T>::type>::value) {
            return MYSQL_TYPE_DOUBLE;
        } else if (is_same<char, typename decay<T>::type>::value) {
            return MYSQL_TYPE_STRING;
        } else if (is_same<signed char, typename decay<T>::type>::value
            || is_same<unsigned char, typename decay<T>::type>::value) {
            return MYSQL_TYPE_TINY;
        }
        return MYSQL_TYPE_STRING;
    }    
    template<typename T>    
    static inline typename enable_if<is_same<char, typename decay<T>::type>::value, size_t>::type MyLength(T * value) {
        return strlen(value);
    }
    template<typename T>    
    static inline typename enable_if<!is_same<char, typename decay<T>::type>::value, size_t>::type MyLength(T *) {
        return sizeof(T);
    }
    static inline int ExecuteSql(MYSQL_STMT * stmt, uint64_t & insertId) {
        if (mysql_stmt_execute(stmt) != 0) {
            return -1;
        }
        insertId = mysql_stmt_insert_id(stmt);
        return (int)mysql_stmt_affected_rows(stmt);
    }
    template<typename T, int N>
    static inline int QuerySql(MYSQL_STMT * stmt, vector<T> & result) {
        MYSQL_RES * res = mysql_stmt_result_metadata(stmt);
        if (res == nullptr) {
            return -1;
        }
        unsigned int count = mysql_num_fields(res);
        mysql_free_result(res);

        if (count == 0 || mysql_stmt_execute(stmt) != 0
            || mysql_stmt_store_result(stmt) != 0) {
            return -1;
        }
        MYSQL_BIND *bind = (MYSQL_BIND *)calloc(count, sizeof(MYSQL_BIND));
        if (bind == nullptr) {
            return -1;
        }
        Defer  guard([&bind]{
            delete bind;
        });
        T    data;
        data.Construct(bind, integral_constant<int, N>());
        if (mysql_stmt_bind_result(stmt, bind) != 0) {
            return -1;
        }
        while (true) {
            int fetch = mysql_stmt_fetch(stmt);
            if (fetch == 1) {
                return -1;
            } else if (fetch == MYSQL_NO_DATA) {
                break;
            } else {
                result.push_back(data);
                data.Construct(bind, integral_constant<int, N>());
            }
        }
        return 0;
    }
    static inline int PrepareSql(MYSQL_STMT * stmt, const string & sqlStr, vector<MYSQL_BIND> & params) {
        if (mysql_stmt_prepare(stmt, sqlStr.c_str(), sqlStr.size()) != 0) {
            return -1;
        }
        if (!params.empty() && mysql_stmt_bind_param(stmt, &params[0]) != 0) {
            return -1;
        }
        return 0;
    }
    template<typename T>
    static inline void Bind(vector<MYSQL_BIND> & params, unsigned long size, T *buffer) {
        MYSQL_BIND      param;
        memset(&param, 0, sizeof(param));
        param.buffer_type = MyType(buffer);
        param.buffer_length = size; 
        params.emplace_back(param);
    }
};
};

#endif
