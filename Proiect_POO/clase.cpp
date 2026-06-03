#include "clase.h"

using namespace std;

// =========================================================================
// IMPLEMENTARE CLASA DE BAZĂ: Carte
// =========================================================================

// Constructorul clasei de bază - utilizează Lista de Inițializare pentru performanță maximă
Carte::Carte(string id, string isbn, string autor, string titlu, bool stare, string data, string user)
    : id(id), isbn(isbn), autor(autor), titlu(titlu), inBiblioteca(stare), dataAparitiei(data), cineAImprumutat(user) {}

// Returnează un string formatat cu caracterele de control "\r\n" necesare pentru afișarea corectă în controlul EDIT din Windows
string Carte::getDetalii() const {
    return "[" + id + "] " + titlu + " - " + autor + "\r\n" +
           "    ISBN: " + isbn + " | An: " + dataAparitiei + "\r\n" +
           "    Stare: " + (inBiblioteca ? "DISPONIBILA" : "IMPRUMUTATA") + "\r\n";
}

// Încarcă atributele citind secvențial dintr-un stream de fișier text delimitat prin ';'
void Carte::incarca(ifstream& in) {
    string stareStr;
    getline(in, id, ';');
    getline(in, isbn, ';');
    getline(in, autor, ';');
    getline(in, titlu, ';');
    getline(in, stareStr, ';');
    getline(in, dataAparitiei, ';');
    getline(in, cineAImprumutat, '\n'); // Sfârșitul liniei se termină cu caracterul newline '\n'

    inBiblioteca = (stareStr == "1"); // Convertim din string ("1"/"0") în tipul boolean intern (true/false)
}

// Salvează obiectul curent în fișier respectând perfect sintaxa bazei noastre de date CSV
void Carte::salveaza(ofstream& out) const {
    out << "CARTE;" << id << ";" << isbn << ";" << autor << ";" << titlu << ";"
        << (inBiblioteca ? "1" : "0") << ";" << dataAparitiei << ";" << (inBiblioteca ? "nimeni" : cineAImprumutat) << "\n";
}

// =========================================================================
// IMPLEMENTARE CLASA DERIVATĂ: CarteTehnica
// =========================================================================

// Constructorul clasei derivate - pasează argumentele comune către constructorul clasei de bază Carte
CarteTehnica::CarteTehnica(string id, string isbn, string autor, string titlu, bool stare, string data, string limbaj, string user)
    : Carte(id, isbn, autor, titlu, stare, data, user), limbajProgramare(limbaj) {}

// Extinde polimorfic metoda getDetalii() din clasa de bază, adăugând linia specifică limbajului de programare
string CarteTehnica::getDetalii() const {
    // Apelăm metoda clasei de bază prin Carte::getDetalii() și concatenăm atributul specific
    return Carte::getDetalii() + "    Specific: Carte tehnica [" + limbajProgramare + "]\r\n";
}

// Parsarea specifică pentru cărțile tehnice (conține un token în plus în structura fișierului: limbajProgramare)
void CarteTehnica::incarca(ifstream& in) {
    string stareStr;
    getline(in, id, ';');
    getline(in, isbn, ';');
    getline(in, autor, ';');
    getline(in, titlu, ';');
    getline(in, stareStr, ';');
    getline(in, dataAparitiei, ';');
    getline(in, limbajProgramare, ';'); // <-- Atribut specific citit din fișier
    getline(in, cineAImprumutat, '\n');

    inBiblioteca = (stareStr == "1");
}

// Salvează obiectul tehnic în fișier adăugând markerul de tip "TEHNICA" și stringul de limbaj
void CarteTehnica::salveaza(ofstream& out) const {
    out << "TEHNICA;" << id << ";" << isbn << ";" << autor << ";" << titlu << ";"
        << (inBiblioteca ? "1" : "0") << ";" << dataAparitiei << ";" << limbajProgramare << ";" << (inBiblioteca ? "nimeni" : cineAImprumutat) << "\n";
}
