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

#ifndef __engine_h__
#define __engine_h__

#include <stdint.h>
#include "mysql.h"
#include "kfifo.h"
#include <string>
using namespace std;
using namespace Libmv;

namespace MySqlOrm {
struct EngHost {
    string          host;
    string          user;
    string          passwd;
    string          db;
    unsigned int    port;
    string          socket;
    unsigned long   flag;
    EngHost() : flag(0), port(3306) {}
    EngHost(const string& h, const string& u, const string& p, const string& d, unsigned int P = 3306) 
        : host(h), user(u), passwd(p), db(d), port(P), flag(0) {}
    EngHost(const EngHost& other) 
        : host(other.host), user(other.user), passwd(other.passwd), db(other.db)
        , port(other.port), socket(other.socket), flag(other.flag) {}
};

struct MyHdl {
    MYSQL       *conn;
    MYSQL_STMT  *stmt;
    MyHdl() : conn(nullptr), stmt(nullptr) {}
};

class Engine {
public:
    explicit Engine(const EngHost & eng, uint32_t poolSize) {
        m_host = eng;
        m_fifo = new Kfifo<MyHdl>(poolSize);
        if (m_fifo == nullptr) {
            return;
        }
        for (int i = 0; i < (int)poolSize; i++) {
            m_fifo->Enqueue(MyHdl());
        }        
    }
    
    ~Engine() {
        if (m_fifo != nullptr) {
            MyHdl        hdl;
            while (m_fifo->Dequeue(hdl) == 0) {
                Engine::Close(hdl);                
            }
            delete m_fifo;
        }
    }

    MyHdl GetHdl(bool locked = true) {
        MyHdl            hdl;
        if (m_fifo != nullptr) {
            m_fifo->Dequeue(hdl, locked);
            if (hdl.conn != nullptr) {
                return hdl;
            }
        }
        
        MYSQL *conn = mysql_init(nullptr);
        if (conn != nullptr) {
            if (nullptr == mysql_real_connect(conn, 
                    (m_host.host.empty()) ? nullptr : m_host.host.c_str(), 
                    (m_host.user.empty()) ? nullptr : m_host.user.c_str(), 
                    (m_host.passwd.empty()) ? nullptr : m_host.passwd.c_str(), 
                    (m_host.db.empty()) ? nullptr : m_host.db.c_str(), 
                    m_host.port,
                    (m_host.socket.empty()) ? nullptr : m_host.socket.c_str(), 
                    m_host.flag)) {
                mysql_close(conn);
                return hdl;
            }
            MYSQL_STMT *stmt = mysql_stmt_init(conn);
            if (stmt == nullptr) {
                mysql_close(conn);
                return hdl;
            }
            hdl.conn = conn;
            hdl.stmt = stmt;
        }        
        return hdl;
    }

    void RetHdl(MyHdl & hdl, bool locked = true) {
        int         ret   = -1;
        if (m_fifo != nullptr) {
            ret = m_fifo->Enqueue(hdl, locked);
        }
        if (ret == -1) {
            Engine::Close(hdl);
        }
    }

private:
    static inline void Close(MyHdl & hdl) {
        if (hdl.conn != nullptr) {
            mysql_close(hdl.conn);
            hdl.conn = nullptr;
        }
        if (hdl.stmt != nullptr) {
            mysql_stmt_close(hdl.stmt);
            hdl.stmt = nullptr;
        }
    }
    Kfifo<MyHdl>            *m_fifo;
    EngHost                 m_host;
};
};

#endif

