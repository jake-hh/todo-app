# Dokumentacja inteligentna tablica

Zadanie jest inspirowane kontenerem vector z biblioteki standardowej.

Inteligentna tablica będzie mogła przechowywać dane dowolnego typu. Tablica będzie potrafiła dostosowywać swój rozmiar do liczby przechowywanych elementów.

Liczba przechowywanych elementów nie jest powiązana z miejscem zarezerwowanym. Tablica może mieć zarezerwowane więcej miejsca do przechowywania elementów, niż faktycznie przechowywanych. Przykładowo tablica może mieć zarezerwowane miejsce dla 100 elementów, ale przechowywać tylko 10. Po dodaniu kolejnego elementu będzie przechowywać 11 elementów, ale nadal będzie mieć miejsce zarezerwowane dla 100. Jeżeli będziemy przechowywać 100 elementów i dla 100 elementów będzie zarezerwowane miejsce, to będzie należało zwiększyć ilość zarezerwowanego miejsca, poprzez realokację elementów (elementy mają być trzymane w jednym ciągu w pamięci). Reguła realokacji (o ile elementów należy zwiększać pojemność) należy samemu dobrać.

> **Informacja** Tworząc implementację biblioteki nie stosuj kontenerów oraz smart pointerów!

> **Ostrzeżenie** Nie załączaj przestrzeni nazw std!

> **Ważne** Biblioteka będzie sprawdzana za pomocą testów jednostkowych! W szablonie projektu znajduje się przygotowana struktura do tego.


## Założenia

- Tablica przechowuje dane dowolnego (jednego) typu
- Tablica potrafi zwiększać swój rozmiar w razie potrzeby
- Tablica automatycznie nie zmniejsza swojego rozmiaru (zmniejsza się tylko liczba przechowywanych elementów)

Dokonaj odpowiedniego podziału na pliki tworząc implementację biblioteki.


## Funkcjonalności

Tablica musi posiadać zaimplementowane poniższe funkcjonalności z dokładnie takimi nazwami oraz parametrami. Jeżeli zostaną użyte inne nazwy albo parametry, co będzie się wiązało z błędami kompilacji testów, to będzie wiązało się to z poprawieniem oraz przyjściem na kolejny termin oddawania. Można dokonywać jedynie zmian, które nie wpływają na sposób wywoływania funkcji, przykładowo zmiany optymalizacyjne (np. T -> const T &).

Warto stworzyć odpowiednie metody, jako const, aby móc ich używać w obiektach, które będą const (dodatkowe).

Wewnątrz tablicy można implementować dowolne dodatkowe metody (publiczne, czy też prywatne).

Zaimplementuj dostęp do elementów tak, aby można było je modyfikować wewnątrz tablicy. Nie używaj do tego celu wskaźników.


## Wymagane publiczne metody

### Wymagania na 3.0

- Konstruktor bezparametryczny - tworzy pustą tablicę
- Konstruktor `(unsigned size)` - tworzy tablicę z zarezerwowaną pamięcią dla `size` elementów ale wielkością 0
- `T at(unsigned index)` - zwraca element pod indeksem `index` albo wyrzuca wyjątek `std::out_of_range`
- `T operator[](unsigned index)` - zwraca element pod indeksem `index` (bez sprawdzania, czy jest w zakresie)
- `bool isEmpty()` - sprawdza czy tablica jest pusta (`true` jeżeli jest pusta)
- `unsigned size()` - zwraca liczbę elementów przechowywanych w tablicy
- `unsigned capacity()` - zwraca liczbę elementów, dla których została zarezerwowana pamięć
- `void reserve(unsigned size)` - rezerwuje pamięć dla `size` elementów, ale nie zwiększa liczby przechowywanych elementów
- `void resize(unsigned size)`, `void resize(unsigned size, T elem)` - zmiana rozmiaru tablicy (nie pojemności). Zmniejszanie, jeżeli zbyt duża, powiększanie, jeżeli zbyt mała (w tym przypadku może się zwiększyć pojemność). Wypełnianie domyślnymi wartościami albo `elem`
- `void clear()` - usuwanie wszystkich elementów, rozmiar -> 0, pojemność nie zmienia się
- `void insert(unsigned index, T elem)` - wstawianie elementu `elem` pod podany indeks `index`, przesuwanie pozostałych elementów albo wyrzuca wyjątek `std::out_of_range`
- `void erase(unsigned index)` - usuwanie elementu spod podanego indeksu `index` albo wyrzuca wyjątek `std::out_of_range`
- `void erase(unsigned index, unsigned amount)` - usuwanie podanej liczby elementów (amount) zaczynając od indeksu `index` albo wyrzuca wyjątek `std::out_of_range`
- `void pushBack(T elem)` - dodawanie elementu na koniec tablicy
- `void popBack()` - usuwanie ostatniego elementu z tablicy (pojemność zostaje ta sama, rozmiar się zmienia)

### Wymagania na 4.0

- Konstruktor `(unsigned size, T elem)` - tworzy tablicę wypełnioną `size` elementami o wartości `elem`
- `bool operator==(SmartArray array)` - sprawdzanie, czy tablice są takie same (wszystkie elementy przechowywane są takie same)

### Wymagania na 5.0

- `void shrinkToFit()` - dostosowuje pojemność tablicy do aktualnie przechowywanej liczby elementów
- `void pushFront(T elem)` - dodawanie elementu na początek tablicy
- `void popFront()` - usuwanie elementu z początku tablicy
- wypisywanie zawartości tablicy do strumienia `cout` (dodatkowe)

> **Ostrzeżenie** Należy zaimplementować niewypisane funkcjonalności, które są konieczne do prawidłowego funkcjonowania klasy bez awarii aplikacji.

