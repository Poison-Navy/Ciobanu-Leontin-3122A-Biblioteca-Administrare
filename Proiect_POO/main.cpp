#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <sstream>
#include <uxtheme.h> // Librărie nativă Windows pentru aplicarea stilurilor și temelor moderne
#include "clase.h"

// Structură simplă folosită pentru maparea și stocarea conturilor de utilizatori în memoria RAM
struct Utilizator {
    std::string username;
    std::string parola;
    std::string rol; // Poate fi "admin" sau "client"
};

// =========================================================================
// VARIABILE GLOBALE (Starea aplicației și Pointerii către elementele UI)
// =========================================================================
std::vector<Carte*> biblioteca;          // Vectorul polimorfic central ce reține baza de date de cărți
std::vector<Utilizator> listaUtilizatori; // Vectorul ce reține toate conturile înregistrate
size_t indexUtilizatorLogat = 0;         // Reține poziția în vector a utilizatorului curent (pentru profil/setări)
std::string textCautatGlobal = "";       // Stringul tampon folosit pentru filtrarea indexului în timp real

// Handlere (pointeri de memorie) către ferestrele și controalele grafice din sistem
HWND hMainWnd, hLoginWnd, hSettingsWnd;
HWND hEditAfisare, hInputID, hBtnImprumut, hBtnReturnare, hBtnSalveaza, hBtnGoToSettings, hBtnLogOut, hBtnExit;
HWND hInputUser, hInputPass, hBtnLogin, hBtnGoToRegister;
HWND hRegUser, hRegPass, hRegComboRol, hBtnRegister;
HWND hInputCautare;
HWND hEditGestiuneConturi, hSetUser, hSetPass, hBtnUpdateProfil, hRadioLight, hRadioDark, hInputDelUser, hBtnDelUser;

// Macro-uri numerice folosite ca ID-uri unice pentru interceptarea evenimentelor (click butoane, edit text)
#define ID_BTN_IMPRUMUT     101
#define ID_BTN_RETURNARE    102
#define ID_BTN_SALVEAZA     103
#define ID_BTN_GOTO_SET     104
#define ID_BTN_LOGIN        105
#define ID_BTN_GOTO_REG     106
#define ID_BTN_REGISTER     107
#define ID_BTN_UP_PROFIL    108
#define ID_RADIO_LIGHT      109
#define ID_RADIO_DARK       110
#define ID_BTN_DEL_USER     111
#define ID_INPUT_CAUTARE    112
#define ID_BTN_LOGOUT       113
#define ID_BTN_EXIT         114

// State Flags pentru controlul comportamentului logic și cromatic
bool esteAdmin = false;
bool esteDarkMode = true; // Aplicația pornește implicit optimizată în Dark Mode
bool vreaLogOut = false;  // Flag ce determină dacă la închiderea ferestrei principale revenim la Login sau dăm Exit

// Obiecte grafice de tip GDI (Graphics Device Interface) pentru managementul culorilor
HBRUSH hBrushBack = NULL;  // Pensulă pentru fundalul ferestrelor principale
HBRUSH hBrushEdit = NULL;  // Pensulă pentru fundalul căsuțelor de input (Edit controls)
COLORREF clrText = RGB(240, 240, 240);       // Culoarea textului (Alb fin / Off-white)
COLORREF clrBack = RGB(30, 30, 30);          // Culoarea fundalului general (Gri închis pur)
COLORREF clrControale = RGB(45, 45, 45);     // Culoarea fundalului pentru căsuțele de text
COLORREF clrButon = RGB(65, 65, 65);         // Culoarea de bază a butoanelor

// =========================================================================
// FUNCȚII AUXILIARE & MANAGEMENTUL TEMEI VIZUALE
// =========================================================================

// Actualizează culorile din memorie în funcție de starea flag-ului esteDarkMode
void AplicaTema() {
    if (hBrushBack) DeleteObject(hBrushBack); // Curățăm obiectele vechi din memorie pentru a evita Memory Leak-uri GDI
    if (hBrushEdit) DeleteObject(hBrushEdit);

    if (esteDarkMode) {
        clrText = RGB(245, 245, 245); clrBack = RGB(30, 30, 30); clrControale = RGB(45, 45, 45); clrButon = RGB(65, 65, 65);
    } else {
        clrText = RGB(20, 20, 20); clrBack = RGB(245, 245, 245); clrControale = RGB(255, 255, 255); clrButon = RGB(220, 220, 220);
    }
    hBrushBack = CreateSolidBrush(clrBack); // Instanțiem pensulele solide cu noile culori RGB
    hBrushEdit = CreateSolidBrush(clrControale);

    // Forțăm ferestrele active să își redeseneze întreaga suprafață (InvalidateRect) cu noile culori
    if (hMainWnd) {
        SetWindowTheme(hMainWnd, L"DarkMode_Explorer", NULL); // Aplică stilul Dark nativ din Windows core controls
        InvalidateRect(hMainWnd, NULL, TRUE);
        InvalidateRect(hEditAfisare, NULL, TRUE);
    }
    if (hSettingsWnd) { SetWindowTheme(hSettingsWnd, L"DarkMode_Explorer", NULL); InvalidateRect(hSettingsWnd, NULL, TRUE); }
    if (hLoginWnd) { SetWindowTheme(hLoginWnd, L"DarkMode_Explorer", NULL); InvalidateRect(hLoginWnd, NULL, TRUE); }
}

// Transformă un string în litere mici (folosit pentru căutare insensibilă la majuscule)
std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// Re-randeză catalogul complet de cărți în interiorul controlului text mare din stânga
void ActualizeazaLista() {
    std::string textTotal = esteAdmin ? "=== PANOU ADMINISTRATOR (MOD EDITARE) ===\r\n\r\n" : "=== CATALOG BIBLIOTECA ===\r\n\r\n";
    std::string usernameLogat = listaUtilizatori[indexUtilizatorLogat].username;

    for (const auto& carte : biblioteca) {
        // [POO / Securitate]: Dacă utilizatorul curent este un simplu Client, el nu are voie
        // să vadă cărțile împrumutate de alți clienți unici. Trecem peste ele.
        if (!esteAdmin && !carte->getInBiblioteca() && carte->getCineAImprumutat() != usernameLogat) {
            continue;
        }

        // Extragerea polimorfică a detaliilor (metoda getDetalii() apelează dynamic legătura corectă în funcție de tipul cărții)
        std::string detalii = carte->getDetalii();

        // Logica barei de căutare: dacă textul căutat nu se află în detalii, sărim peste carte
        if (!textCautatGlobal.empty() && ToLower(detalii).find(ToLower(textCautatGlobal)) == std::string::npos) continue;

        // Marcaj vizual contextual pentru clientul care își analizează propria listă
        if (!esteAdmin && carte->getCineAImprumutat() == usernameLogat) {
            textTotal += "[ IMPRUMUTATA DE TINE ] ";
        }
        textTotal += detalii + "--------------------------------------------------------\r\n";
    }
    SetWindowText(hEditAfisare, textTotal.c_str()); // Împingem textul concatenat direct în interfața grafică
}

// Implementarea manuală a butoanelor rotunjite (Owner-Drawn Window Controls)
void DeseneazaButonPersonalizat(DRAWITEMSTRUCT* pDIS, std::string text) {
    HDC hdc = pDIS->hDC; // Contextul de dispozitiv grafic pentru desenare pe ecran
    RECT rect = pDIS->rcItem; // Coordonatele geometrice ale suprafeței butonului

    // Desenăm o margine fină
    HPEN hPen = CreatePen(PS_SOLID, 1, esteDarkMode ? RGB(55, 55, 55) : RGB(200, 200, 200));
    HGDIOBJ oldPen = SelectObject(hdc, hPen);

    // Schimbăm culoarea dacă butonul detectează stare de apăsare (Click prelungit)
    COLORREF culoareCurentaBtn = clrButon;
    if (pDIS->itemState & ODS_SELECTED) culoareCurentaBtn = esteDarkMode ? RGB(90, 90, 90) : RGB(170, 170, 170);

    HBRUSH hBrushBtn = CreateSolidBrush(culoareCurentaBtn);
    HGDIOBJ oldBrush = SelectObject(hdc, hBrushBtn);

    // Generăm forma de buton rotunjit geometric cu o rază de curbură de 10x10 pixeli
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 10, 10);

    // Randarea și centrarea perfectă a etichetei de text peste corpul butonului rotunjit
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, clrText);
    DrawText(hdc, text.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Curățarea corectă a obiectelor temporare din pipeline-ul GDI pentru a preveni căderile de performanță
    SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen);
    DeleteObject(hBrushBtn); DeleteObject(hPen);
}

// =========================================================================
// MANAGEMENTUL FIȘIERELOR ȘI LOGICA BANTELOR DE DATE
// =========================================================================

// Încarcă utilizatorii din utilizatori.txt sau generează baza implicită extinsă dacă fișierul lipsește
void IncarcaUtilizatori() {
    listaUtilizatori.clear();
    std::ifstream in("utilizatori.txt");
    if (!in) {
        // Script de populare automată la prima rulare (1 Admin + 10 Clienți Reali unici)
        listaUtilizatori.push_back({"admin", "admin", "admin"});
        listaUtilizatori.push_back({"mihai_popescu", "1234", "client"});
        listaUtilizatori.push_back({"elena_ionescu", "5678", "client"});
        listaUtilizatori.push_back({"andrei_radu", "pass123", "client"});
        listaUtilizatori.push_back({"maria_sandu", "mar123", "client"});
        listaUtilizatori.push_back({"gabi_dumitru", "gabi99", "client"});
        listaUtilizatori.push_back({"cristi_vlad", "cristi88", "client"});
        listaUtilizatori.push_back({"laura_stan", "laura2026", "client"});
        listaUtilizatori.push_back({"bogdan_marin", "bogdan1", "client"});
        listaUtilizatori.push_back({"anca_dinu", "anca_d", "client"});
        listaUtilizatori.push_back({"stefan_lupu", "stefan9", "client"});

        // Salvăm structura generată pentru a fi disponibilă la rulările viitoare
        std::ofstream out("utilizatori.txt");
        if (out) {
            for (const auto& ut : listaUtilizatori) out << ut.username << " " << ut.parola << " " << ut.rol << "\n";
            out.close();
        }
        return;
    }
    std::string u, p, r;
    while (in >> u >> p >> r) listaUtilizatori.push_back({u, p, r});
    in.close();
}

// Rescrie integral utilizatori.txt cu starea curentă din vector (pentru modificări parole/ștergeri conturi)
void SalveazaToateConturile() {
    std::ofstream out("utilizatori.txt");
    if (out) {
        for (const auto& ut : listaUtilizatori) out << ut.username << " " << ut.parola << " " << ut.rol << "\n";
        out.close();
    }
}

// Interogare corelată (Admin Statistic): Calculează dinamic ce cărți are fiecare cont în posesie
void ActualizeazaGestiuneConturi() {
    if (!hEditGestiuneConturi) return;

    std::string text = "=== SITUATIE IMPRUMUTURI CLIENTI ===\r\n\r\n";

    for (const auto& ut : listaUtilizatori) {
        if (ut.rol == "client") {
            int cartiNumarate = 0;
            std::string listaCartiDetaliata = "";

            // Căutare în baza centrală de cărți pe baza numelui de utilizator asociat stării de împrumut
            for (const auto& carte : biblioteca) {
                if (!carte->getInBiblioteca() && carte->getCineAImprumutat() == ut.username) {
                    cartiNumarate++;
                    listaCartiDetaliata += " [ID: " + carte->getId() + "]"; // Concatenăm ID-urile găsite
                }
            }

            text += "Utilizator: " + ut.username + "\r\n";
            text += " -> Total carti imprumutate: " + std::to_string(cartiNumarate);
            if (cartiNumarate > 0) text += " " + listaCartiDetaliata;
            text += "\r\n--------------------------------------------\r\n";
        }
    }
    SetWindowText(hEditGestiuneConturi, text.c_str());
}

// =========================================================================
// PROCEDURILE DE EVENIMENT WINDOWS (CALLBACK FUNCTIONS)
// =========================================================================

// FEREASTRA PRINCIPALĂ (Catalogul și panoul de comenzi)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
            return 0;

        // Evenimente GDI: Trimise de Windows înainte de a desena textele fixe (Label-urile)
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, clrText);
            SetBkColor(hdc, clrBack);
            return (LRESULT)hBrushBack; // Returnăm pensula de fundal unificată
        }
        // Trimis de Windows înainte de a desena interiorul casetelor editabile
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, clrText);
            SetBkColor(hdc, clrControale);
            return (LRESULT)hBrushEdit;
        }

        // Rută specială: Prinde butoanele marcate ca BS_OWNERDRAW și le redirecționează spre funcția noastră de rotunjire
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*)lParam;
            if (pDIS->CtlID == ID_BTN_IMPRUMUT) DeseneazaButonPersonalizat(pDIS, "Imprumuta Carte");
            if (pDIS->CtlID == ID_BTN_RETURNARE) DeseneazaButonPersonalizat(pDIS, "Returneaza Carte");
            if (pDIS->CtlID == ID_BTN_GOTO_SET) DeseneazaButonPersonalizat(pDIS, "Setari Profil");
            if (pDIS->CtlID == ID_BTN_SALVEAZA) DeseneazaButonPersonalizat(pDIS, "SALVEAZA CARTI");
            if (pDIS->CtlID == ID_BTN_LOGOUT) DeseneazaButonPersonalizat(pDIS, "Log Out");
            if (pDIS->CtlID == ID_BTN_EXIT) DeseneazaButonPersonalizat(pDIS, "Iesire Aplicatie");
            return TRUE;
        }

        // Interceptarea click-urilor și modificărilor text din interfață
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);

            // Căutare asincronă: se declanșează instantaneu când utilizatorul modifică textul în căsuța de căutare (EN_CHANGE)
            if (wmId == ID_INPUT_CAUTARE && HIWORD(wParam) == EN_CHANGE) {
                char bufCautare[64]; GetWindowText(hInputCautare, bufCautare, 64);
                textCautatGlobal = std::string(bufCautare);
                ActualizeazaLista(); // Refiltrare și re-randare instatanee
            }

            // Comenzi de modificare stare (Împrumut / Returnare)
            if (wmId == ID_BTN_IMPRUMUT || wmId == ID_BTN_RETURNARE) {
                char bufID[16]; GetWindowText(hInputID, bufID, 16); std::string idCautat(bufID);
                for (auto& carte : biblioteca) {
                    if (carte->getId() == idCautat) {
                        if (wmId == ID_BTN_IMPRUMUT) {
                            carte->setStare(false); // Setează starea ca împrumutată (0)
                            // Leagă cartea polimorfic de numele contului curent logat
                            carte->setCineAImprumutat(listaUtilizatori[indexUtilizatorLogat].username);
                        } else {
                            carte->setStare(true);  // Resetare în bibliotecă (1)
                            carte->setCineAImprumutat("nimeni");
                        }
                        break;
                    }
                }
                ActualizeazaLista(); SetWindowText(hInputID, ""); // Reîmprospătare text interfață și curățare input
            }

            // Salvare explicită din memorie în fișierul fizic text
            if (wmId == ID_BTN_SALVEAZA) {
                std::ofstream out("biblioteca.txt");
                if (out) {
                    for (const auto& carte : biblioteca) carte->salveaza(out); // Apelare polimorfică a metodei virtuale salveaza()
                    out.close();
                    MessageBox(hwnd, "Fisier salvat!", "Salvat", MB_OK);
                }
            }

            // Gestionare Log Out securizat
            if (wmId == ID_BTN_LOGOUT) {
                vreaLogOut = true; // Setăm flag-ul pe adevărat
                DestroyWindow(hwnd); // Spargem fereastra principală; bucla din WinMain va intercepta ieșirea și va redeschide login-ul
                return 0;
            }

            // Gestionare Închidere Aplicație direct din interfață
            if (wmId == ID_BTN_EXIT) {
                vreaLogOut = false;
                PostQuitMessage(0); // Închide complet întregul proces .exe
                return 0;
            }

            // GENERARE DINAMICĂ A PANOUULUI DE SETĂRI (La apăsarea butonului)
            if (wmId == ID_BTN_GOTO_SET) {
                if (hSettingsWnd) { SetFocus(hSettingsWnd); return 0; } // Dacă fereastra e deja deschisă, o aducem în față

                int inaltimeSetari = esteAdmin ? 450 : 220; // Adminul are nevoie de o fereastră mai mare pentru panoul statistic de clienți
                hSettingsWnd = CreateWindowEx(0, "SettingsWinClass", "Panou Setari", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 550, inaltimeSetari, hwnd, NULL, NULL, NULL);
                SetWindowTheme(hSettingsWnd, L"DarkMode_Explorer", NULL);

                // Grupul și butoanele radio pentru management Light/Dark mode
                HWND hG1 = CreateWindowEx(0, "BUTTON", "Stil Fundal", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 10, 240, 60, hSettingsWnd, NULL, NULL, NULL);
                hRadioLight = CreateWindowEx(0, "BUTTON", "Light Mode", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 25, 30, 100, 20, hSettingsWnd, (HMENU)ID_RADIO_LIGHT, NULL, NULL);
                hRadioDark = CreateWindowEx(0, "BUTTON", "Dark Mode", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, 130, 30, 100, 20, hSettingsWnd, (HMENU)ID_RADIO_DARK, NULL, NULL);
                SendMessage(esteDarkMode ? hRadioDark : hRadioLight, BM_SETCHECK, BST_CHECKED, 0); // Bifează radio-ul activ conform stării aplicației

                // Controale pentru modificarea datelor profilului propriu (Username / Parolă)
                HWND hG2 = CreateWindowEx(0, "BUTTON", "Detalii Profil Curent", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 15, 80, 240, 120, hSettingsWnd, NULL, NULL, NULL);
                CreateWindowEx(0, "STATIC", "User:", WS_CHILD | WS_VISIBLE, 25, 100, 50, 20, hSettingsWnd, NULL, NULL, NULL);
                hSetUser = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", listaUtilizatori[indexUtilizatorLogat].username.c_str(), WS_CHILD | WS_VISIBLE, 80, 100, 160, 20, hSettingsWnd, NULL, NULL, NULL);
                CreateWindowEx(0, "STATIC", "Pass:", WS_CHILD | WS_VISIBLE, 25, 130, 50, 20, hSettingsWnd, NULL, NULL, NULL);
                hSetPass = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", listaUtilizatori[indexUtilizatorLogat].parola.c_str(), WS_CHILD | WS_VISIBLE, 80, 130, 160, 20, hSettingsWnd, NULL, NULL, NULL);
                hBtnUpdateProfil = CreateWindowEx(0, "BUTTON", "Modifica", WS_CHILD | WS_VISIBLE, 80, 160, 160, 25, hSettingsWnd, (HMENU)ID_BTN_UP_PROFIL, NULL, NULL);

                // Aplicăm designul unificat Dark nativ pe noile sub-controale ferestre copii
                SetWindowTheme(hG1, L"DarkMode_Explorer", NULL); SetWindowTheme(hG2, L"DarkMode_Explorer", NULL);
                SetWindowTheme(hRadioLight, L"DarkMode_Explorer", NULL); SetWindowTheme(hRadioDark, L"DarkMode_Explorer", NULL);

                // Logica specifică Admin: construim interfața din dreapta pentru monitorizarea conturilor de clienți
                if (esteAdmin) {
                    hEditGestiuneConturi = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | WS_BORDER, 270, 15, 250, 300, hSettingsWnd, NULL, NULL, NULL);
                    CreateWindowEx(0, "STATIC", "Sterge User Client:", WS_CHILD | WS_VISIBLE, 270, 330, 120, 20, hSettingsWnd, NULL, NULL, NULL);
                    hInputDelUser = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE, 270, 355, 130, 25, hSettingsWnd, NULL, NULL, NULL);
                    hBtnDelUser = CreateWindowEx(0, "BUTTON", "Sterge Cont", WS_CHILD | WS_VISIBLE, 410, 355, 110, 25, hSettingsWnd, (HMENU)ID_BTN_DEL_USER, NULL, NULL);
                    SetWindowTheme(hBtnDelUser, L"DarkMode_Explorer", NULL);
                    ActualizeazaGestiuneConturi(); // Populează raportul statistic de împrumuturi
                }
                ShowWindow(hSettingsWnd, SW_SHOW);
            }
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
        default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// FEREASTRA DE SETĂRI (Procedura callback secundară)
LRESULT CALLBACK SettingsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CTLCOLORSTATIC: { HDC hdc = (HDC)wParam; SetTextColor(hdc, clrText); SetBkColor(hdc, clrBack); return (LRESULT)hBrushBack; }
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLOREDIT: { HDC hdc = (HDC)wParam; SetTextColor(hdc, clrText); SetBkColor(hdc, clrControale); return (LRESULT)hBrushEdit; }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            // Comutare dinamică temă Light/Dark
            if (wmId == ID_RADIO_LIGHT) { esteDarkMode = false; AplicaTema(); }
            if (wmId == ID_RADIO_DARK) { esteDarkMode = true; AplicaTema(); }

            // Salvare modificări profil propriu
            if (wmId == ID_BTN_UP_PROFIL) {
                char user[32], pass[32]; GetWindowText(hSetUser, user, 32); GetWindowText(hSetPass, pass, 32);
                if (strlen(user) > 0 && strlen(pass) > 0) {
                    listaUtilizatori[indexUtilizatorLogat].username = user; listaUtilizatori[indexUtilizatorLogat].parola = pass;
                    SalveazaToateConturile(); MessageBox(hwnd, "Profil modificat!", "Succes", MB_OK);
                }
            }
            // Logică Admin: Ștergerea unui cont de client din listă și din baza de date
            if (wmId == ID_BTN_DEL_USER && esteAdmin) {
                char userDel[32]; GetWindowText(hInputDelUser, userDel, 32); std::string sUserDel(userDel);
                for (auto it = listaUtilizatori.begin(); it != listaUtilizatori.end(); ++it) {
                    if (it->username == sUserDel && it->rol == "client") {
                        listaUtilizatori.erase(it); // Eliminare element din structura vector
                        SalveazaToateConturile(); // Rescriere fișier
                        ActualizeazaGestiuneConturi(); SetWindowText(hInputDelUser, ""); return 0;
                    }
                }
            }
            return 0;
        }
        case WM_DESTROY: hSettingsWnd = NULL; return 0; // Resetare pointer la închidere
        default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// FEREASTRA DE LOGIN (Procedura callback)
LRESULT CALLBACK LoginProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CTLCOLORSTATIC: { HDC hdc = (HDC)wParam; SetTextColor(hdc, clrText); SetBkColor(hdc, clrBack); return (LRESULT)hBrushBack; }
        case WM_CTLCOLOREDIT: { HDC hdc = (HDC)wParam; SetTextColor(hdc, clrText); SetBkColor(hdc, clrControale); return (LRESULT)hBrushEdit; }
        case WM_COMMAND: {
            // Verificare credențiale la apăsarea butonului Log In
            if (LOWORD(wParam) == ID_BTN_LOGIN) {
                char user[32], pass[32]; GetWindowText(hInputUser, user, 32); GetWindowText(hInputPass, pass, 32);
                for (size_t i = 0; i < listaUtilizatori.size(); i++) {
                    if (listaUtilizatori[i].username == std::string(user) && listaUtilizatori[i].parola == std::string(pass)) {
                        esteAdmin = (listaUtilizatori[i].rol == "admin"); // Setăm drepturile globale de acces conform rolului stocat
                        indexUtilizatorLogat = i;
                        DestroyWindow(hwnd); // Distrugem ecranul de login; deblocăm firul principal către WinMain
                        return 0;
                    }
                }
                MessageBox(hwnd, "Date incorecte!", "Eroare", MB_OK);
            }
            // Creare și afișare Panou Înregistrare Cont Nou (Register Window)
            if (LOWORD(wParam) == ID_BTN_GOTO_REG) {
                HWND hRegWnd = CreateWindowEx(0, "RegisterWinClass", "Cont Nou", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 350, 240, hwnd, NULL, NULL, NULL);
                SetWindowTheme(hRegWnd, L"DarkMode_Explorer", NULL);
                CreateWindowEx(0, "STATIC", "User:", WS_CHILD | WS_VISIBLE, 20, 20, 100, 20, hRegWnd, NULL, NULL, NULL);
                hRegUser = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE, 130, 20, 170, 25, hRegWnd, NULL, NULL, NULL);
                CreateWindowEx(0, "STATIC", "Parola:", WS_CHILD | WS_VISIBLE, 20, 60, 100, 20, hRegWnd, NULL, NULL, NULL);
                hRegPass = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_PASSWORD, 130, 60, 170, 25, hRegWnd, NULL, NULL, NULL);
                hRegComboRol = CreateWindowEx(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 130, 100, 170, 100, hRegWnd, NULL, NULL, NULL);
                SendMessage(hRegComboRol, CB_ADDSTRING, 0, (LPARAM)"Client"); SendMessage(hRegComboRol, CB_ADDSTRING, 0, (LPARAM)"Administrator"); SendMessage(hRegComboRol, CB_SETCURSEL, 0, 0);
                HWND hb = CreateWindowEx(0, "BUTTON", "Inregistrare", WS_CHILD | WS_VISIBLE, 130, 145, 170, 35, hRegWnd, (HMENU)ID_BTN_REGISTER, NULL, NULL);
                SetWindowTheme(hb, L"DarkMode_Explorer", NULL); ShowWindow(hRegWnd, SW_SHOW);
            }
            return 0;
        }
        case WM_CLOSE: PostQuitMessage(0); return 0; // Oprește aplicația complet dacă se închide forțat fereastra de login
        default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// FEREASTRA DE ÎNREGISTRARE (Procedura callback)
LRESULT CALLBACK RegisterProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CTLCOLORSTATIC: { HDC hdc = (HDC)wParam; SetTextColor(hdc, clrText); SetBkColor(hdc, clrBack); return (LRESULT)hBrushBack; }
        case WM_CTLCOLOREDIT: { HDC hdc = (HDC)wParam; SetTextColor(hdc, clrText); SetBkColor(hdc, clrControale); return (LRESULT)hBrushEdit; }
        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_BTN_REGISTER) {
                char user[32], pass[32], rol[16]; GetWindowText(hRegUser, user, 32); GetWindowText(hRegPass, pass, 32);
                int sel = SendMessage(hRegComboRol, CB_GETCURSEL, 0, 0); SendMessage(hRegComboRol, CB_GETLBTEXT, sel, (LPARAM)rol);

                std::string sUser(user), sPass(pass), sRol(rol);
                if (sRol == "Administrator") sRol = "admin"; else sRol = "client"; // Mapare sintaxă internă

                if (sUser.empty() || sPass.empty()) return 0;
                listaUtilizatori.push_back({sUser, sPass, sRol}); // Inserare în vector
                SalveazaToateConturile(); // Salvare pe disc
                MessageBox(hwnd, "Cont creat!", "Succes", MB_OK); DestroyWindow(hwnd);
            }
            return 0;
        }
        default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// =========================================================================
// PUNCTUL DE INTRARE ÎN PROGRAM (APPLICATION ENTRY POINT)
// =========================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    // 1. Inițializarea și încărcarea structurilor de date de pe disc în memoria RAM
    IncarcaUtilizatori();
    AplicaTema();

    std::ifstream in("biblioteca.txt"); std::string tip;
    if (in) {
        while (getline(in, tip, ';')) { // Citire bazată pe token-ul polimorfic central
            Carte* c = nullptr;
            if (tip == "CARTE") c = new Carte();
            else if (tip == "TEHNICA") c = new CarteTehnica();

            if (c) {
                c->incarca(in); // Apelare polimorfică virtuală; încarcă configurarea corectă din fișier
                biblioteca.push_back(c);
            }
        }
        in.close();
    }

    // 2. Înregistrarea claselor geometrice de ferestre în nucleul sistemului de operare Windows
    WNDCLASS wl = {}, wr = {}, wc = {}, ws = {};
    wl.lpfnWndProc = LoginProc; wl.hInstance = hInstance; wl.lpszClassName = "LoginWinClass"; wl.hCursor = LoadCursor(NULL, IDC_ARROW); wl.hbrBackground = hBrushBack; RegisterClass(&wl);
    wr.lpfnWndProc = RegisterProc; wr.hInstance = hInstance; wr.lpszClassName = "RegisterWinClass"; wr.hCursor = LoadCursor(NULL, IDC_ARROW); wr.hbrBackground = hBrushBack; RegisterClass(&wr);
    wc.lpfnWndProc = WindowProc; wc.hInstance = hInstance; wc.lpszClassName = "GestiuneBibliotecaGUI"; wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = hBrushBack; RegisterClass(&wc);
    ws.lpfnWndProc = SettingsProc; ws.hInstance = hInstance; ws.lpszClassName = "SettingsWinClass"; ws.hCursor = LoadCursor(NULL, IDC_ARROW); ws.hbrBackground = hBrushBack; RegisterClass(&ws);

    // 🔄 ETICHETĂ LOGICĂ: Punctul unde programul sare înapoi când utilizatorul dă click pe [Log Out]
    PornireSistem:
    vreaLogOut = false;

    // 3. Crearea și instanțierea ferestrei grafice de LOGIN
    hLoginWnd = CreateWindowEx(0, "LoginWinClass", "Autentificare", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 350, 240, NULL, NULL, hInstance, NULL);
    SetWindowTheme(hLoginWnd, L"DarkMode_Explorer", NULL);
    CreateWindowEx(0, "STATIC", "Utilizator:", WS_CHILD | WS_VISIBLE, 30, 20, 80, 20, hLoginWnd, NULL, NULL, NULL);
    hInputUser = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE, 120, 20, 180, 25, hLoginWnd, NULL, NULL, NULL);
    CreateWindowEx(0, "STATIC", "Parola:", WS_CHILD | WS_VISIBLE, 30, 60, 80, 20, hLoginWnd, NULL, NULL, NULL);
    hInputPass = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_PASSWORD, 120, 60, 180, 25, hLoginWnd, NULL, NULL, NULL);
    HWND hl1 = CreateWindowEx(0, "BUTTON", "Log In", WS_CHILD | WS_VISIBLE, 120, 105, 180, 35, hLoginWnd, (HMENU)ID_BTN_LOGIN, NULL, NULL);
    HWND hl2 = CreateWindowEx(0, "BUTTON", "Cont Nou", WS_CHILD | WS_VISIBLE, 120, 150, 180, 30, hLoginWnd, (HMENU)ID_BTN_GOTO_REG, NULL, NULL);
    SetWindowTheme(hl1, L"DarkMode_Explorer", NULL); SetWindowTheme(hl2, L"DarkMode_Explorer", NULL);

    ShowWindow(hLoginWnd, nCmdShow);

    // Mini-bucla blocantă de mesaje dedicată exclusiv ferestrei de Login
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); if (!IsWindow(hLoginWnd)) break; }
    if (msg.message == WM_QUIT) return 0; // Dacă s-a dat X direct la login, oprim executabilul complet

    // 4. Instanțierea Fereastrei Principale a Catalogului (Unificată geometric la 800px lățime)
    hMainWnd = CreateWindowEx(0, "GestiuneBibliotecaGUI", esteAdmin ? "PANOU CENTRAL - ADMINISTRATOR" : "CATALOG - CLIENT", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 800, 580, NULL, NULL, hInstance, NULL);
    SetWindowTheme(hMainWnd, L"DarkMode_Explorer", NULL);

    // Creare controale zonă căutare rapidă și listă mare de afișare cu bară de scroll vertical activă
    CreateWindowEx(0, "STATIC", "Cauta rapid (ID / Titlu / Autor / ISBN):", WS_CHILD | WS_VISIBLE, 10, 12, 280, 20, hMainWnd, NULL, NULL, NULL);
    hInputCautare = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE, 300, 10, 210, 25, hMainWnd, (HMENU)ID_INPUT_CAUTARE, NULL, NULL);
    hEditAfisare = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_BORDER, 10, 45, 500, 485, hMainWnd, NULL, hInstance, NULL);

    // Generarea butoanelor cu flag-ul BS_OWNERDRAW pentru activarea stilului personalizat rotunjit
    hBtnGoToSettings = CreateWindowEx(0, "BUTTON", "Setari Profil", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 530, 395, 230, 38, hMainWnd, (HMENU)ID_BTN_GOTO_SET, NULL, NULL);
    hBtnLogOut       = CreateWindowEx(0, "BUTTON", "Log Out", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 530, 440, 230, 38, hMainWnd, (HMENU)ID_BTN_LOGOUT, NULL, NULL);
    hBtnExit         = CreateWindowEx(0, "BUTTON", "Iesire Aplicatie", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 530, 485, 230, 38, hMainWnd, (HMENU)ID_BTN_EXIT, NULL, NULL);

    // [POO / Polimorfism]: Injectăm controalele administrative suplimentare pe ecran doar dacă flag-ul esteAdmin este validat
    if (esteAdmin) {
        CreateWindowEx(0, "STATIC", "Introduceti ID Carte:", WS_CHILD | WS_VISIBLE, 530, 45, 230, 20, hMainWnd, NULL, NULL, NULL);
        hInputID = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_NUMBER, 530, 70, 230, 25, hMainWnd, NULL, NULL, NULL);
        hBtnImprumut = CreateWindowEx(0, "BUTTON", "Imprumuta Carte", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 530, 110, 230, 38, hMainWnd, (HMENU)ID_BTN_IMPRUMUT, NULL, NULL);
        hBtnReturnare = CreateWindowEx(0, "BUTTON", "Returneaza Carte", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 530, 155, 230, 38, hMainWnd, (HMENU)ID_BTN_RETURNARE, NULL, NULL);
        hBtnSalveaza = CreateWindowEx(0, "BUTTON", "SALVEAZA CARTI", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 530, 350, 230, 38, hMainWnd, (HMENU)ID_BTN_SALVEAZA, NULL, NULL);
    }

    // Alinierea temei și popularea inițială a listei cu datele proaspăt parsate
    AplicaTema();
    ActualizeazaLista();
    ShowWindow(hMainWnd, nCmdShow);

    // 5. Bucla Principală de Mesaje a întregului sistem grafic (Main Message Pump)
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }

    // Interceptare Log Out: Dacă ieșirea din buclă s-a făcut prin comutare profil, sărim înapoi la eticheta de start
    if (vreaLogOut) goto PornireSistem;

    // 6. CURĂȚAREA MEMORIEI (Destructori): Rulați automat la închidere totală a aplicației
    if (hBrushBack) DeleteObject(hBrushBack);
    if (hBrushEdit) DeleteObject(hBrushEdit);
    for (auto& c : biblioteca) delete c; // Distrugere dinamică polimorfică (eliberează instanțele generate cu new)

    return 0;
}
