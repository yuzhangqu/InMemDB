#include <iostream>
#include <string>
#include "TransMessage.h"
#include "TransMessage.pb.h"

using std::string;
using std::cin;
using std::cout;
using std::endl;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "You should specify the server address." << endl;
        return 1;
    }
    gpb *buf = new gpb();
    gpb *result;
    InMemDB::TransRequest trans;
    InMemDB::TransRequest::Op *op;
    CLIENT *cl;
    cl = clnt_create(argv[1], DBTRANS, DBTRANS_V1, "tcp");
    if (cl == NULL) {
        cout << "Could not connect to server" << endl;
        return 1;
    }
    string tmp;
    int key, key2;
    while (true) {
        cout << "Enter an operation code (or leave blank to finish): ";
        getline(cin, tmp);
        if (tmp.compare("put") == 0 || tmp.compare("get") == 0) {
            op = trans.add_op();
            if (tmp.compare("put") == 0) {
                op->set_code(InMemDB::TransRequest_Op_Code_PUT);
            }
            else {
                op->set_code(InMemDB::TransRequest_Op_Code_GET);
            }
            cout << "Enter a key: ";
            cin >> key;
            op->set_key(key);
            cin.ignore(256, '\n');
            if (tmp.compare("put") == 0) {
                cout << "Enter a value: ";
                getline(cin, *op->mutable_value());
            }
        }
        else if (tmp.compare("getrange") == 0) {
            op = trans.add_op();
            op->set_code(InMemDB::TransRequest_Op_Code_GETRANGE);
            cout << "Enter a start position: ";
            cin >> key;
            op->set_key(key);
            cin.ignore(256, '\n');
            cout << "Enter an end position: ";
            cin >> key2;
            op->set_key2(key2);
            cin.ignore(256, '\n');
        }
        else {
            break;
        }
    }
    if (trans.op_size() < 1) {
        cout << "No operation, exit now." << endl;
        return 1;
    }
    tmp.clear();
    trans.SerializeToString(&tmp);
    buf->size = tmp.size();
    buf->data = (char*)malloc(tmp.size());
    memcpy(buf->data, tmp.c_str(), tmp.size());
    result = dbaccess_1(buf, cl);
    if (result == NULL) {
        cout << "error:RPC failed" << endl;
        return 1;
    }
    string tmp_rsp (result->data, result->size);
    InMemDB::TransResponse trans_rsp;
    trans_rsp.ParseFromString(tmp_rsp);
    for (int i = 0; i < trans_rsp.ret_size(); i++) {
        const InMemDB::TransResponse::Ret &ret = trans_rsp.ret(i);
        cout << "Key: " << ret.key() << ", value: " << ret.value() << endl;
    }
    return 0;
}

