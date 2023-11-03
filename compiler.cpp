#include <iostream>
#include <cstring>
#include <map>
#include <exception>
#include <fstream>

#include "compiler.hpp"

using namespace std;

// コンストラクタ
Compiler::Compiler(void) {}

// プログラムの文字列のセット
void Compiler::set_program_string(string str) {
    this->str = str;
    cursor = &this->str[0];
}

// プログラムファイルのセット
void Compiler::set_program_file(string filename) {
    // ファイルを開く
    ifstream file(filename);

    // ファイルの内容を格納する文字列
    string file_contents;

    if (file.is_open()) {   // ファイルが正常に開かれた場合
        string line;

        // ファイルの内容を1行ずつ読み込んで文字列に追加
        while (getline(file, line)) {
            file_contents += line + "\n"; // 改行を追加
        }

        // ファイルを閉じる
        file.close();

    } else {    // ファイルを開けなかった場合
        cerr << "ファイルを開けませんでした。" << endl;
        exit(1);
    }

    set_program_string(file_contents);
}

// コンパイル
void Compiler::compile(void) {
    program();
}

// プログラム (program)
void Compiler::program(void) {
    statement();
    for (;;) {
        if (*cursor == '.') {
            break;
        } else if (*cursor == ';') {
            cursor++;
        } else {
            syntax_error("文の末尾が正しく認識されません．");
        }
        statement();
    }
}

// 文 (statement)
void Compiler::statement(void) {
    skip();
    if (parse_read()) {
        ;
    } else if (parse_write()) {
        ;
    } else if (parse_assignment()) {
        ;
    } else if (parse_while_do()) {
        ;
    } else if (*cursor == ';' || *cursor == '.') {
        ;
    } else {
        syntax_error("文が正しく認識されません．");
    }
    skip();
}

// READ文（statement内）
bool Compiler::parse_read(void) {
    string str = "READ";
    int len = str.length();

    // カーソルの文字がREADと一致するか
    if (strncmp(cursor, str.c_str(), len) == 0) {
        cursor += len;  // 一致したら文字数の分だけカーソルを進める
    } else {
        return false;
    }

    // 左括弧の解析
    if (*cursor == '(') {
        cursor++;
    } else {
        syntax_error("READ文の左括弧が認識されません．");
    }

    // 引数の解析
    skip(); // READ文の変数名の前に空白がある場合の処理
    if (isupper(*cursor)) {
        obj.push_back(Command(GET, int(*cursor - 'A')));    // [GET カーソルの文字のメモリ番地]をobjに追加
        cursor++;
    } else {
        syntax_error("READ文の引数が正しくありません．");
    }
    skip(); // READ文の変数名の後に空白がある場合の処理

    // 右括弧の解析
    if (*cursor == ')') {
        cursor++;
    } else {
        syntax_error("READ文の右括弧が認識されません．");
    }

    return true;
}

// WRITE文（statement内）
bool Compiler::parse_write(void) {
    string str = "WRITE";
    int len = str.length();

    // カーソルの文字がWRITEと一致するか
    if (strncmp(cursor, str.c_str(), len) == 0) {
        cursor += len;  // 一致したら文字数の分だけカーソルを進める
    } else {
        return false;
    }

    // 左括弧の解析
    if (*cursor == '(') {
        cursor++;
    } else {
        syntax_error("WRITE文の左括弧が認識されません．");
    }

    // 引数の解析
    skip(); // WRITE文の変数名の前に空白がある場合の処理
    if (isupper(*cursor)) {
        obj.push_back(Command(PUT, int(*cursor - 'A')));    // [PUT カーソルの文字のメモリ番地]をobjに追加
        cursor++;
    } else {
        syntax_error("WRITE文の引数が正しくありません．");
    }
    skip(); // WRITE文の変数名の後に空白がある場合の処理

    // 右括弧の解析
    if (*cursor == ')') {
        cursor++;
    } else {
        syntax_error("WRITE文の右括弧が認識されません．");
    }

    return true;
}

// 代入文（statement内）
bool Compiler::parse_assignment(void) {
    int operand;    // 代入文の左辺のメモリ番地の保存用
    char* tmp_cursor = cursor;  // falseの場合のロールバック用

    // 左辺の解析
    if (isupper(*cursor)) {
        operand = *cursor - 'A';
        cursor++;
    } else {
        return false;
    }

    // :=の解析
    skip(); // :=の前に空白がある場合の処理
    string str = ":=";
    int len = str.length();
    if (strncmp(cursor, str.c_str(), len) == 0) {   // カーソルの文字が:=と一致するか
        cursor += len;
    } else {
        cursor = tmp_cursor;    // ロールバック
        return false;
    }

    // 右辺の解析
    simple_expression();

    // [STR 左辺のメモリ番地]が追加される
    obj.push_back(Command(STR, operand));

    return true;
}

bool Compiler::parse_while_do(void) {
    string str = "WHILE";
    int len = str.length();

    // カーソルの文字がWHILEと一致するか
    if (strncmp(cursor, str.c_str(), len) == 0) {
        cursor += len;  // 一致したら文字数の分だけカーソルを進める
    } else {
        return false;
    }
    if (*cursor == ' ') {
        cursor++;
    } else {
        syntax_error("WHILE文の後に空白がありません．");
    }
    skip(); // WHILE文の後に空白が複数ある場合の処理

    // 関係式の解析時点のプログラムカウンタの保存（UJPのオペランドになる）
    int ujp_counter = obj.size();
    // 関係式の解析
    relational_expression();

    // DOの解析
    str = "DO";
    len = str.length();
    if (strncmp(cursor, str.c_str(), len) == 0) {
        cursor += len;
    } else {
        syntax_error("WHILE文のDOが認識されません．");
    }

    if (*cursor == ' ' || *cursor == '\n') {
        cursor++;
    } else {
        syntax_error("WHILE文のDOの後に空白または改行がありません．");
    }
    skip(); // WHILE文のDOの後に空白または改行が複数ある場合の処理

    // 関係式の解析後時点のプログラムカウンタの保存（CJPの挿入先になる）
    int cjp_counter = obj.size();

    str = "ENDWHILE";
    len = str.length();
    statement();
    for(;;) {
        if (*cursor == ';') {
            cursor++;
        } else if (strncmp(cursor, str.c_str(), len) == 0) {
            cursor += len;
            break;
        } else {
            syntax_error("WHILE文の終わり(ENDWHILE)が認識されません．");
        }
        statement();
    }
    obj.push_back(Command(UJP, ujp_counter));   // [UJP 関係式の解析前のプログラムカウンタ]がobjに追加される

    // [CJP WHILE文終了直後のプログラムカウンタ]がobjのcjp_counterの部分に挿入される
    obj.insert(obj.begin()+cjp_counter, Command(CJP, obj.size()+1));

    return true;
}

// 関係式 (relational expression)
void Compiler::relational_expression(void) {
    simple_expression();
    if (*cursor == '=') {
        cursor++;
        simple_expression();
        obj.push_back(Command(EQL, nullopt));
    } else if (*cursor == '>') {
        cursor++;
        simple_expression();
        obj.push_back(Command(GRT, nullopt));
    } else if (*cursor == '<') {
        cursor++;
        simple_expression();
        obj.push_back(Command(LET, nullopt));
    } else {
        syntax_error("関係式が正しくありません．");
    }
}

// 単純式 (simple expression)
void Compiler::simple_expression(void) {
    term();
    for (;;) {
        if (*cursor == '+') {
            cursor++;
            term();
            obj.push_back(Command(ADD, nullopt));
            continue;
        } else if (*cursor == '-') {
            cursor++;
            term();
            obj.push_back(Command(SUB, nullopt));
            continue;
        }
        break;
    }
}

// 項 (term)
void Compiler::term(void) {
    factor();
    for (;;) {
        if (*cursor == '*') {
            cursor++;
            factor();
            obj.push_back(Command(MLT, nullopt));
            continue;
        } else if (*cursor == '/') {
            cursor++;
            factor();
            obj.push_back(Command(DIV, nullopt));
            continue;
        }
        break;
    }
}

// 因子 (factor)
void Compiler::factor(void) {
    skip();

    if (*cursor == '(') {
        cursor++;
        simple_expression();
        if (*cursor == ')') {
            cursor++;
        }
    } else if (isdigit(*cursor)) {
        unsigned_constant();
    } else if (isupper(*cursor)) {
        obj.push_back(Command(LOD, int(*cursor - 'A')));
        cursor++;
    } else {
        syntax_error("因子が正しくありません．");
    }

    skip();
}

// 符号なし定数 (unsigned constant)
void Compiler::unsigned_constant(void) {
    string sb = "";
    bool first_call = true;
    while (isdigit(*cursor)) {
        if (first_call && *cursor == '0') {
            syntax_error("定数が0から始まっています．");
        }
        if (first_call) {
            first_call = false;
        }
        sb.push_back(*cursor);
        cursor++;
    }
    obj.push_back(Command(LDC, stoi(sb)));
}

// 空白文字と改行コードの読み飛ばし
void Compiler::skip(void) {
    while (*cursor == ' ' || *cursor == '\n') {
        cursor++;
    }
}

// 構文エラー
void Compiler::syntax_error(string message) {
    cerr << "構文エラー: " << message << endl;
    for (char* c = &str[0]; c != &str.back(); c++) {
        if (c == cursor) {
            cerr << "\033[1;41m" << *c << "\033[m";
        } else {
            cerr << *c;
        }
    }
    cerr << endl;

    exit(1);
}

// ニモニックの列挙型と文字列の対応表
map<int, string> Compiler::mnemonic2string_map = {
    {GET, "GET"},
    {PUT, "PUT"},
    {LOD, "LOD"},
    {LDC, "LDC"},
    {STR, "STR"},
    {ADD, "ADD"},
    {SUB, "SUB"},
    {MLT, "MLT"},
    {DIV, "DIV"},
    {EQL, "EQL"},
    {GRT, "GRT"},
    {LET, "LET"},
    {CJP, "CJP"},
    {UJP, "UJP"},
};

// 結果の表示
void Compiler::print_result(void) {
    for (auto i : obj) {
        if (i.operand.has_value()) {
            cout << Compiler::mnemonic2string_map[i.mnemonic] << ' ' << i.operand.value() << endl;
        } else {
            cout << Compiler::mnemonic2string_map[i.mnemonic] << endl;
        }
    }
}

// ファイルに結果を出力
void Compiler::output_file(string filename) {
    // ファイルを開く
    ofstream file(filename);

    if (file.is_open()) {   // ファイルが正常に開かれた場合
        // 結果の出力
        for (auto i : obj) {
            if (i.operand.has_value()) {
                file << Compiler::mnemonic2string_map[i.mnemonic] << ' ' << i.operand.value() << endl;
            } else {
                file << Compiler::mnemonic2string_map[i.mnemonic] << endl;
            }
        }

        // ファイルを閉じる
        file.close();

    } else {    // ファイルを開けなかった場合
        cerr << "ファイルを開けませんでした。" << endl;
        exit(1);
    }
}