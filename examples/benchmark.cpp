#include <vector>
#include <algorithm>
#include "session.h"
#include <iostream>
#include <chrono>
using namespace MySqlOrm;
using namespace std;
using namespace chrono;

#define MAX 100
struct Students {
    Column<int64_t>      id;
    Column<int64_t>      uniqid;
    Column<int16_t>      type;
    Column<string, 32>   session;
    Column<string, 512>   name;
    Column<int32_t>      age;
    Column<int32_t>      max;
    Column<string, 256>   nick;

    _DEFINE(Students, _FIELD(id), _FIELD(uniqid), _FIELD(type), _FIELD(session), _FIELD(name), _FIELD(age), max.name = "max(id)", _FIELD(nick))
    _CONSTRUCT(0, max);
};

int main(int argc, char **argv) {
    if (argc != 2) {
        cout << "Useage:" << argv[0] << " nums" << endl;
        return -1;
    }
    int  nums = atoi(argv[1]);
    const EngHost   host("127.0.0.1", "root", "", "test", 3306);
    Engine      *eng = new Engine(host, 10);
    if (eng == nullptr) {
        return -1;
    }
    Session    se;
    se.Bind(eng);
    Students    student;
    int ret = 0;
    ret = se.Text("drop table Students").Execute();
    if (ret < 0) {
        cout << "drop table err:" << se.Errstr()<< endl;
    }
    se.Print();
    ret = se.Text("CREATE TABLE Students ("\
            "id int(20) NOT NULL AUTO_INCREMENT,"\
            "uniqid int(20) NOT NULL,"\
            "type int(5) NOT NULL DEFAULT '0',"\
            "session varchar(32) NOT NULL,"\
            "name varchar(512) NOT NULL,"\
            "age int(10) DEFAULT '0',"\
            "nick varchar(256) DEFAULT ' ',"\
            "KEY idx_session(session),"\
            "UNIQUE KEY idx_uniqid (uniqid),"\
            "PRIMARY KEY (id))").Execute();
    if (ret < 0) {
        cout << "create table err:" << se.Errstr()<< endl;
    }
    
    char  name[512] = {0};
    for (int i = 0; i < 51; i++) {
        for (int j = 0; j < 10; j++) {
             name[i * 10 + j] = j + '0';
        }
    }
    char nick[256] = {0};
    for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 10; j++) {
            nick[i * 10 + j] = j + 'A';
        }
    }
    srand(time(NULL));
    char session[32] = "AAbd1245456";
    auto start = steady_clock::now();
    int count = 0;
    se.Table("Students");
    for (int j = 0; j < nums / MAX; j++) {
    se.Insert(student.uniqid, student.type, student.session, student.name, student.age, student.nick);
    se.Values<true>(rand(), 1, session, name, 38, nick);
    for (int i = 0; i < MAX - 1; i++) {
        session[i / 10] = i % 10 + '0';
        se(rand(), 1, session, name, 38, nick);
    }
    se.OnDupKey(student.id.Value());
    ret = se.Execute(); 
    if (ret < 0) {
        se.Print();
        cout << "insert err:" << se.Errstr()<< endl;
        return -1;
    }
    count += ret;
    }
    int timeOut = (duration_cast<milliseconds>(steady_clock::now() - start)).count();
    cout << "time is:" << timeOut << " ms" << endl;
    vector<Students>  v;
    ret = se.Query<Students, 0>().GetAll(v);
    if (ret < 0) {
        cout << "select err:" << se.Errstr()<< endl;
        return -1;
    }
    for_each(v.begin(), v.end(), [count](Students s) {
        cout << "count:" << s.max.val << "-" << count << endl;
    }); 
    return 0;
}
