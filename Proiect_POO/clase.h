#ifndef CLASE_H
#define CLASE_H

#include <string>
#include <vector>
#include <fstream>

// =========================================================================
// CLASA DE BAZĂ: Carte (Implementează conceptul de Încapsulare)
// =========================================================================
class Carte {
protected:
    // Atribute protejate: pot fi accesate direct doar de această clasă și de clasele derivate (ex: CarteTehnica)
    std::string id;
    std::string isbn;
    std::string autor;
    std::string titlu;
    bool inBiblioteca;
    std::string dataAparitiei;
    std::string cineAImprumutat; // Reține username-ul clientului care a împrumutat cartea

public:
    // Constructor implicit (default) - necesar pentru alocarea dinamică inițială
    Carte() = default;

    // Constructor cu parametri pentru inițializarea completă a obiectului
    Carte(std::string id, std::string isbn, std::string autor, std::string titlu, bool stare, std::string data, std::string user = "nimeni");

    // [POO - Polimorfism]: Metode virtuale. Permite claselor derivate să își schimbe comportamentul în timpul rulării (Runtime)
    virtual std::string getDetalii() const;
    virtual void incarca(std::ifstream& in);
    virtual void salveaza(std::ofstream& out) const;

    // Getteri și Setteri (Metode inline pentru asigurarea conceptului de Încapsulare a datelor membre)
    std::string getId() const { return id; }
    bool getInBiblioteca() const { return inBiblioteca; }
    void setStare(bool stare) { inBiblioteca = stare; }

    std::string getCineAImprumutat() const { return cineAImprumutat; }
    void setCineAImprumutat(std::string user) { cineAImprumutat = user; }

    // [POO - Bună practică]: Destructor virtual obligatoriu pentru a preveni Memory Leak-urile la ștergerea polimorfică
    virtual ~Carte() {}
};

// =========================================================================
// CLASA DERIVATĂ: CarteTehnica (Implementează conceptul de Moștenire)
// =========================================================================
class CarteTehnica : public Carte {
private:
    // Atribut privat: specific doar cărților de profil IT
    std::string limbajProgramare;

public:
    // Constructor implicit
    CarteTehnica() = default;

    // Constructor cu parametri care apelează și constructorul clasei de bază
    CarteTehnica(std::string id, std::string isbn, std::string autor, std::string titlu, bool stare, std::string data, std::string limbaj, std::string user = "nimeni");

    // [POO - Polimorfism]: Suprascrierea metodelor din clasa de bază prin cuvântul cheie 'override'
    std::string getDetalii() const override;
    void incarca(std::ifstream& in) override;
    void salveaza(std::ofstream& out) const override;
};

#endif // CLASE_H
