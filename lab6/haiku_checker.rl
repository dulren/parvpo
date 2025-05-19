#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <sstream>
using namespace std;

static vector<string> words = {
    "кот", "власть", "ВЕТЕР", "кит", "ночь",
    "рис", "штат", "клад", "краб", "лес",
    "дар", "снег", "брат", "тень", "мир"
};

string generate_line(int count) {
    string s;
    for (int i = 0; i < count; ++i) {
        if (i) s += " ";
        s += words[rand() % words.size()];
    }
    return s;
}

string generate_haiku() {
    stringstream ss;
    ss << generate_line(5) << "\n";
    ss << generate_line(7) << "\n";
    ss << generate_line(5) << "\n\n";
    return ss.str();
}

int vc;


%%{
    machine haiku;

    action vowel  { vc++; }
    action check1 { if (vc != 5) cs = haiku_error; vc = 0;}
    action check2 { if (vc != 7) cs = haiku_error; vc = 0;}
    action check3 { if (vc != 5) cs = haiku_error; vc = 0;}

    vowel_char = ( 'а' | 'е' | 'ё' | 'и' | 'о' | 'у' | 'ы' | 'э' | 'ю' | 'я'
                 | 'А' | 'Е' | 'Ё' | 'И' | 'О' | 'У' | 'Ы' | 'Э' | 'Ю' | 'Я' ) @vowel;

    non_nl = any - '\n';

    line1 = (vowel_char | non_nl)* '\n' @check1;
    line2 = (vowel_char | non_nl)* '\n' @check2;
    line3 = (vowel_char | non_nl)* '\n' @check3;

    main := line1 line2 line3 '\n';
}%%

extern "C" int verify_haiku(const string& input) {
    const char *p  = input.c_str();
    const char *pe = p + input.size();
    int cs;

    %% write data;   
    vc = 0;         
    %% write init;   
    %% write exec;   

    return (cs >= haiku_first_final && p == pe) ? 1 : 0;
}

int main() {
    srand((unsigned)time(nullptr));
    for (int i = 1; i <= 10; ++i) {
        string h = generate_haiku();
        if (verify_haiku(h)) cout << i << " VALID " << h;
        else                  cout << i << " INVALID " << h;
    }
    return 0;
}
