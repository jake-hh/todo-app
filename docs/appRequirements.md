# Wymagania ogólne

- Projekt musi być zrealizowany z wykorzystaniem CMake
- Projekt musi składać się z 2 części:
	- Biblioteki - inteligentna tablica
	- Aplikacji

**Informacje**
- Biblioteka będzie sprawdzana za pomocą testów jednostkowych (gtest)
- Projekt będzie sprawdzany przy użyciu kompilatora Visual Studio!

## Biblioteka

Wymagania biblioteki zostały zawarte w `docs/smartArrayRequirements.md`

## Aplikacja

Aplikacja musi zostać zrealizowany przy użyciu TUI (Terminal User Interface), albo GUI (Graphical User Interface).

Aplikacja będzie wykorzystywać co najmniej jedną instancję stworzonej biblioteki (smartArray). Zadaniem aplikacji będzie obsługa jakimś zbiorem struktur danych. Przykładowo:

- Zarządzanie pracownikami
- Zarządzanie pojazdami
- Zarządzanie liniami
- Zarządzanie studentami
- Zarządzanie książkami
- Zarządzanie zajęciami

Gdzie zarządzanie oznacza:

- Dodawanie
- Usuwanie
- Wyświetlanie
- Modyfikowanie
- Dokonywanie jakichś operacji (zlicz, zsumuj, oblicz średnią)
- Zapisz oraz wczytaj z pliku

### Funkcjonalności wymagane na 3.0

- Klasa przechowująca dane musi składać się z co najmniej 5 elementów (minimum 1 string oraz minimum 1 liczba).
- Możliwość dodawania nowych elementów.
- Możliwość usuwania elementów.
- Możliwość edycji elementów.
- Wyświetlenie wszystkich elementów.
- Wyświetlenie liczby przechowywanych elementów.
- Poprawny zapis oraz odczyt elementów z pliku binarnego.

### Funkcjonalności wymagane na 4.0

- Wszystko co na 3.0.
- Wyświetlanie elementów z zakresu (podawane będą numery indeksu).
- Wyświetlanie elementu spod konkretnego indeksu.
- Wyświetlanie skróconej listy elementów - numer indeksu i tylko najważniejsze pole (np - Nazwiska, albo Imienia i Nazwiska)
- Dokumentacja doxygen (wystarczy w kodzie, nie trzeba jej generować).

### Funkcjonalności wymagane na 5.0

- Wszystko co na 4.0.
- Wyszukiwanie elementów (po jednym, albo kilku polach).
- Funkcje agregujące (np. suma wartości któregoś pola, albo średnia).
- Zaawansowane funkcje agregujące (np. suma tylko elementów spełniających jakieś kryterium)

### Oceniane elementy

- Spełnienie wymagań z dokumentacji biblioteki
- Spełnienie wymagań aplikacji
- Poprawność implementacji
- Odpowiedni podział na pliki
- Czystość kodu
- Znajomość kodu
- Działanie aplikacji (czy nie ulegnie awarii)

