#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
using namespace std;

const int SYMBOLS = 131;                 // ASCII 0 to 127 and extra 3 for the pairs ll, tt, nt
const int MAX_NODES = SYMBOLS * 2;
const string SPECIAL[3] = {"ll", "tt", "nt"};

struct Node {
    long long frequency;
    int left, right, symbol, minimumSymbol, order;
};

string symbolText(int id) {
    if (id >= 128) return SPECIAL[id - 128];
    return string(1, static_cast<char>(id));
}

string visible(int id) {
    if (id == 32) return "[space]";
    if (id == 9) return "[tab]";
    if (id == 13) return "[CR]";
    if (id == 10) return "[LF]";
    if (id == 127) return "[DEL]";
    if (id < 32) return "[control ASCII " + to_string(id) + "]";
    if (id >= 128) return "[" + SPECIAL[id - 128] + "]";
    return string(1, static_cast<char>(id));
}

int pairAt(const string& input, size_t i) {
    if (i + 1 >= input.size()) return -1;
    if (input[i] == 'l' && input[i + 1] == 'l') return 128;
    if (input[i] == 't' && input[i + 1] == 't') return 129;
    if (input[i] == 'n' && input[i + 1] == 't') return 130;
    return -1;
}

void countTokens(const string& s, long long freq[SYMBOLS], bool seen[SYMBOLS]) {
    for (int i = 0; i < SYMBOLS; ++i) { freq[i] = 0; seen[i] = false; }
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 128) seen[c] = true;
    }
    for (size_t i = 0; i < s.size();) {
        int p = pairAt(s, i);
        if (p >= 0) {
            ++freq[p]; seen[p] = true;
            i += 2;
        } else {
            ++freq[static_cast<unsigned char>(s[i])];
            ++i;
        }
    }
}

bool comesBefore(const Node& a, const Node& b) {
    if (a.frequency != b.frequency) return a.frequency < b.frequency;
    if (a.minimumSymbol != b.minimumSymbol) return a.minimumSymbol < b.minimumSymbol;
    return a.order < b.order;
}

int smallestActive(Node nodes[MAX_NODES], bool active[MAX_NODES], int n) {
    int smallest = -1;
    for (int i = 0; i < n; ++i)
        if (active[i] && (smallest == -1 || comesBefore(nodes[i], nodes[smallest])))
            smallest = i;
    return smallest;
}

void codesFromTree(const Node nodes[MAX_NODES], int at, const string& path,
                   string codes[SYMBOLS]) {
    if (nodes[at].symbol >= 0) {
        codes[nodes[at].symbol] = path.empty() ? "0" : path;
        return;
    }
    codesFromTree(nodes, nodes[at].left, path + "0", codes);
    codesFromTree(nodes, nodes[at].right, path + "1", codes);
}

void buildCodes(const long long freq[SYMBOLS], const bool seen[SYMBOLS],
                string codes[SYMBOLS]) {
    Node nodes[MAX_NODES];
    bool active[MAX_NODES] = {};
    int count = 0;
    for (int id = 0; id < SYMBOLS; ++id) {
        codes[id] = "";
        if (seen[id]) {
            nodes[count] = {freq[id], -1, -1, id, id, count};
            active[count++] = true;
        }
    }
    if (count == 0) return;
    while (true) {
        int a = smallestActive(nodes, active, count);
        active[a] = false;
        int b = smallestActive(nodes, active, count);
        if (b == -1) { active[a] = true; codesFromTree(nodes, a, "", codes); break; }
        active[b] = false;
        int left = a, right = b;
        if (nodes[b].frequency > nodes[a].frequency ||
            (nodes[b].frequency == nodes[a].frequency &&
             nodes[b].minimumSymbol < nodes[a].minimumSymbol)) {
            left = b; right = a;
        }
        int minSymbol = nodes[a].minimumSymbol < nodes[b].minimumSymbol
                        ? nodes[a].minimumSymbol : nodes[b].minimumSymbol;
        nodes[count] = {nodes[a].frequency + nodes[b].frequency,
                        left, right, -1, minSymbol, count};
        active[count++] = true;
    }
}

string encode(const string& input, const string codes[SYMBOLS]) {
    string bits;
    for (size_t i = 0; i < input.size();) {
        int p = pairAt(input, i);
        if (p >= 0) { bits += codes[p]; i += 2; }
        else { bits += codes[static_cast<unsigned char>(input[i])]; ++i; }
    }
    return bits;
}

bool saveEncoded(const string& filename, const string codes[SYMBOLS],
                 const string& bits) {
    ofstream out(filename.c_str(), ios::binary);
    if (!out) return false;
    int n = 0;
    for (int i = 0; i < SYMBOLS; ++i) if (!codes[i].empty()) ++n;
    out << "HUFFMAN_C11_V1\n" << n << '\n';
    for (int i = 0; i < SYMBOLS; ++i)
        if (!codes[i].empty()) out << i << ' ' << codes[i] << '\n';
    out << bits.size() << '\n' << bits << '\n';
    return static_cast<bool>(out);
}

bool loadAndDecode(const string& filename, string& decoded) {
    ifstream in(filename.c_str(), ios::binary);
    if (!in) return false;
    string magic;
    if (!getline(in, magic) || magic != "HUFFMAN_C11_V1") return false;
    int n;
    if (!(in >> n) || n < 0 || n > SYMBOLS) return false;
    string codes[SYMBOLS];
    for (int j = 0; j < n; ++j) {
        int id; string code;
        if (!(in >> id >> code) || id < 0 || id >= SYMBOLS || !codes[id].empty()) return false;
        for (size_t k = 0; k < code.size(); ++k)
            if (code[k] != '0' && code[k] != '1') return false;
        codes[id] = code;
    }
    size_t length;
    if (!(in >> length)) return false;
    string bits;
    if (!(in >> bits)) {
        if (length != 0) return false;
        in.clear();
    }
    if (bits.size() != length) return false;
    decoded = "";
    string prefix;
    for (size_t i = 0; i < bits.size(); ++i) {
        if (bits[i] != '0' && bits[i] != '1') return false;
        prefix += bits[i];
        int match = -1;
        for (int id = 0; id < SYMBOLS; ++id)
            if (!codes[id].empty() && codes[id] == prefix) {
                match = id; break;
            }
        if (match >= 0) { decoded += symbolText(match); prefix = ""; }
    }
    return prefix.empty();
}

int main() {
    cout << "Enter ASCII text (one line, including spaces and punctuation):\n";
    string input;
    getline(cin, input);
    for (size_t i = 0; i < input.size(); ++i) {
        if (static_cast<unsigned char>(input[i]) > 127) {
            cerr << "Input contains a non-ASCII byte; use ASCII (0-127).\n";
            return 1;
        }
    }
    long long frequency[SYMBOLS];
    bool seen[SYMBOLS];
    string codes[SYMBOLS];
    countTokens(input, frequency, seen);
    buildCodes(frequency, seen, codes);
    cout << "\nSymbol           Frequency   Huffman code\n";
    cout << "-------------------------------------------\n";
    for (int id = 0; id < SYMBOLS; ++id)
        if (seen[id])
            cout << visible(id) << " (ASCII " << id << ")   "
                 << frequency[id] << "   " << codes[id] << '\n';
    string bits = encode(input, codes);
    cout << "\nEncoded text (bits): " << bits << '\n';
    const string filename = "huffman_encoded.txt";
    if (!saveEncoded(filename, codes, bits)) {
        cerr << "Could not save file.\n"; return 1;
    }
    cout << "Saved codes and bitstream to " << filename << '\n';
    string decoded;
    if (!loadAndDecode(filename, decoded)) {
        cerr << "Saved file did not decode correctly.\n"; return 1;
    }
    cout << "Decoded text: " << decoded << '\n';
    cout << "Round-trip check: " << (decoded == input ? "PASS" : "FAIL") << '\n';
    return decoded == input ? 0 : 1;
}
