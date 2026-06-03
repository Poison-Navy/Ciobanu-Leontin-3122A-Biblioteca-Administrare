# 📚 Sistem de Gestiune și Indexare Bibliotecă (POO - Win32 API)

Sistem desktop modern pentru gestionarea unei biblioteci virtuale, dezvoltat în **C++** folosind principiile avansate ale Programării Orientate pe Obiecte (POO) și interfața grafică nativă **Win32 API** (cu suport complet pentru Dark Mode).

---

## ✨ Funcționalități Principale

### 🔒 Sistem Multi-Cont (Autentificare & Înregistrare)
* **Fereastră de Login:** Permite conectarea securizată pe baza datelor stocate în `utilizatori.txt`.
* **Creare Cont:** Panou interactiv pentru generarea de utilizatori noi cu roluri specifice (*Administrator* sau *Client*).

### 👑 Panou Administrator (Mod Gestiune)
* **Filtrare/Căutare în Timp Real:** Căutare instantanee (la fiecare tastare) după ID, Titlu, Autor sau ISBN în indexul de cărți.
* **Operațiuni Împrumut/Returnare:** Modificarea stării cărților prin introducerea ID-ului corespunzător.
* **Salvare Gestiune:** Rescrierea dinamică și permanentă a fișierului `biblioteca.txt`.
* **Statistici Clienți:** Monitorizare în timp real din panoul de setări a numărului de cărți împrumutate de fiecare client și ID-urile acestora.

### 👥 Panou Client (Mod Vizualizare)
* **Securitate la Nivel de Gestiune:** Clientul are acces *Read-Only* la catalog. Butoanele de împrumut/salvare sunt ascunse.
* **Catalog Personalizat:** Clienții pot vedea doar cărțile disponibile în bibliotecă și cărțile împrumutate **de ei înșiși** (marcate cu eticheta `[ IMPRUMUTATA DE TINE ]`). Cărțile altor clienți sunt complet ascunse.

### 🎨 Panou de Setări & Personalizare
* **Teme Vizuale:** Suport complet pentru comutare instantanee între **Light Mode** și **Dark Mode** (Charcoal/Flat UI, fără margini gri învechite, randat nativ prin `uxtheme`).
* **Modificare Profil:** Posibilitatea utilizatorului curent de a-și schimba datele de logare direct din interfață.
* **Ieșire Sigură & Log Out:** Opțiuni dedicate pentru deconectarea de la cont (revenire la Login) sau închidere curată cu eliberare de memorie.

---

## 📖 Ghid de Utilizare al Programului

### 1. Autentificarea în Sistem
* La pornire, aplicația deschide ecranul securizat de **Autentificare**.
* Pentru a testa funcțiile de management, folosiți contul de administrator implicit: `admin` ca utilizator și `admin` ca parolă.
* Pentru a testa catalogul securizat de cititor, folosiți unul dintre cele 10 conturi de clienți stocate în sistem (de exemplu, utilizator: `mihai_popescu`, parolă: `1234`).
* Dacă doriți crearea unui profil personalizat, apăsați pe **Cont Nou**, selectați rolul dorit din meniul derulant (Combobox) și finalizați înregistrarea.

### 2. Utilizarea Catalogului și Căutarea Rapidă
* Fereastra principală conține o casetă de text mare în stânga, unde baza de date polimorfică este randată automat.
* În partea de sus există o casetă numită **Căutați rapid**. Începeți să tastați orice cuvânt cheie (ex: `C++`, `Orwell`, `978` sau chiar un ID precum `12`). Lista se va filtra **automat în timp real** la fiecare literă introdusă, afișând doar elementele potrivite.

### 3. Executarea Împrumuturilor (Doar pentru Administratori)
* Pe panoul lateral din dreapta, introduceți numărul numeric al cărții în căsuța **Introduceți ID Carte** (ex: `1`).
* Apăsați butonul **Împrumută Carte** pentru a o marca ca ocupată sau **Returnează Carte** pentru a o readuce în stocul bibliotecii.
* **IMPORTANT:** Modificările efectuate rămân doar în memoria RAM până când apăsați butonul **SALVEAZA CARTI**. Acesta va scrie permanent noile stări direct în fișierul `biblioteca.txt`.

### 4. Consultarea Statisticilor (Mod Administrator)
* Apăsați butonul **Setări Profil**. Pe lângă schimbarea temei cromatice (Light/Dark), în partea dreaptă va apărea o listă detaliată a clienților.
* Sistemul va calcula instantaneu câte cărți are fiecare cont în posesie și va afișa în paranteze drepte ID-urile volumelor deținute de aceștia. Tot de aici puteți șterge un cont de client scriindu-i numele și apăsând pe **Șterge Cont**.

### 5. Deconectarea și Închiderea Programului
* Din motive de securitate, dacă doriți să schimbați utilizatorul fără a reporni manual executabilul, apăsați pe **Log Out**. Fereastra principală se va închide, iar ecranul de Login va reapărea curat.
* Pentru a părăsi aplicația într-un mod sigur, cu eliberarea integrală a memoriei din vectorul polimorfic, folosiți butonul **Ieșire Aplicație**.

---

## 🛠️ Concepte POO Implementate (Pilonii Principali)

1.  **Încapsulare (Encapsulation):** Datele membre ale cărților (ID, titlu, stare, utilizator curent) sunt protejate (`protected`/`private`) și modificate controlat prin metode de tip Getter și Setter (ex: `getId()`, `getInBiblioteca()`, `setCineAImprumutat()`).
2.  **Moștenire (Inheritance):** Clasa de bază `Carte` este extinsă de clasa derivată `CarteTehnica`, care adaugă atribute specifice precum `limbajProgramare`.
3.  **Polimorfism (Polymorphism):** Metodele `getDetalii()`, `incarca()` și `salveaza()` sunt declarate `virtual` în clasa de bază și sunt **suprascrise (`override`)** în clasa derivată. Colecția este gestionată unitar printr-un vector de pointeri la clasa de bază: `std::vector<Carte*> biblioteca`.
4.  **Gestiune Dinamică a Memoriei:** Alocarea obiectelor în faza de parsare se face utilizând operatorul `new`, iar eliberarea memoriei este tratată explicit la închiderea programului prin utilizarea `delete` în interiorul destructorilor virtuali.

---

## 💾 Structura Bazei de Date (Fișiere Text)

### 1. `biblioteca.txt` (Sintaxă de tip CSV cu separator `;`)
Fiecare linie reprezintă o entitate polimorfică. Structura diferă în funcție de tipul cărții:
* **Carte Literară:** `CARTE;ID;ISBN;Autor;Titlu;Stare(0/1);AnAparitie;CineAImprumutat`
* **Carte Tehnică:** `TEHNICA;ID;ISBN;Autor;Titlu;Stare(0/1);AnAparitie;Limbaj;CineAImprumutat`

*Exemplu:*
```text
CARTE;1;978-0441172719;Frank Herbert;Dune;0;1965;mihai_popescu
TEHNICA;2;978-1491903995;Scott Meyers;Effective Modern C++;1;2014;C++;nimeni
