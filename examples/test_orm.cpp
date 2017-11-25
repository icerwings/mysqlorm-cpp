#include <vector>
#include <algorithm>
#include "session.h"
#include <iostream>
using namespace MySqlOrm;

struct Students {
    Column<int64_t>      id;
    Column<int64_t>      userid;
    Column<int16_t>      type;
    Column<string, 64>   name;
    Column<int32_t>      age;
    Column<int32_t>      sum;
    Column<int32_t>      count;
    Column<string, 32>   nick;

    _DEFINE(Students, _FIELD(id), _FIELD(userid), _FIELD(type), _FIELD(name), _FIELD(age), _SUM(sum, age), _COUNT(count), _FIELD(nick))
    _CONSTRUCT(0, id, userid, type, name, age, sum, count, nick);
    _CONSTRUCT(1, id, userid, type, name, age, nick);
    void Print() {
        cout << id.val << "," << userid.val << "," << type.val << "," << name.val << "," << age.val 
             << "," << sum.val << "," << count.val << "," << nick.val << endl;
    }
};

int main() {
    const EngHost   host("127.0.0.1", "root", "", "test", 3306);
    Engine      *eng = new Engine(host, 10);
    if (eng == nullptr) {
        return -1;
    }
    Session    se;
    se.Bind(eng);
    Students    student;
    int ret = se.Text("drop table Students").Execute();
    if (ret < 0) {
        cout << "drop table err:" << se.Errstr()<< endl;
    }
    se.Print();
    ret = se.Text("CREATE TABLE Students ("\
            "id int(20) NOT NULL AUTO_INCREMENT,"\
            "userid int(20) NOT NULL,"\
            "type int(5) NOT NULL DEFAULT '0',"\
            "name varchar(20) NOT NULL,"\
            "age int(10) DEFAULT '0',"\
            "nick varchar(20) DEFAULT ' ',"\
            "PRIMARY KEY (id),"\
            "UNIQUE KEY idx_userid (userid))").Execute();
    if (ret < 0) {
        cout << "create table err:" << se.Errstr()<< endl;
    }
    se.Print();
    
    se.Table("Students").Insert(student.userid, student.type, student.name, student.age);
    ret = se.Values(10024, 1, "aaa", 38)(10025, 2, "bbb", 40)(10026, 2, "ccc", 38)(10027, 1, "ddd", 38).OnDupKey(student.age + 1, student.id.Value()).Execute();
    if (ret < 0) {
        cout << "insert err:" << se.Errstr()<< endl;
    }
    se.Print();

    uint64_t id = se.InsertId();
    ret = se.Update(student.type = 2, student.nick = "haha").Filter(student.id > id).Execute();
    if (ret < 0) {
        cout << "update err:" << se.Errstr()<< endl;
    }
    se.Print();
    
    cout << "select 1:" << endl;
    vector<Students>  v;
    ret = se.Table("Students").Query<Students, 0>().Groupby(student.type, student.age).GetAll(v);
    if (ret < 0) {
        cout << "select err:" << se.Errstr()<< endl;
    }
    se.Print();
    for_each(v.begin(), v.end(), [](Students s) {
        s.Print();
    });
    cout << "select 2:" << endl;
    v.clear();
    ret = se.Query<Students, 1>().Filter(student.id > 0, _OR(student.type == 1, student.type == 2)).Orderby(student.id).GetAll<Students, 1>(v);
    if (ret < 0) {
        cout << "select err:" << se.Errstr()<< endl;
    }
    se.Print();
    for_each(v.begin(), v.end(), [](Students s) {
        s.Print();
    });
    cout << "select 3:" << endl;
    v.clear();
    ret = se.Text("select id, userid, type, name, age, nick from Students where id > ? and type = ?", 0, 2).GetAll<Students, 1>(v);
    if (ret < 0) {
        cout << "select err:" << se.Errstr()<< endl;
    }
    se.Print();
    for_each(v.begin(), v.end(), [](Students s) {
        s.Print();
    });
    
    return 0;
}
