# SO_6

Biorąc za punkt wyjścia MINIX-owe sterowniki urządzeń umieszczone w katalogu /usr/src/minix/drivers/examples, zaimplementuj sterownik urządzenia /dev/hello_queue działający zgodnie z poniższą specyfikacją.

Działanie urządzenia będzie przypominać prymitywna kolejkę z dodatkowymi operacjami.

W chwili początkowej urządzenie ma dysponować buforem o pojemności DEVICE_SIZE bajtów. Stała ta jest zdefiniowana w dostarczanym przez nas pliku nagłówkowym hello_queue.h. Zastrzegamy możliwość zmiany wartości tej stałej w testach.

Pamięć na bufor powinna być rezerwowana (i zwalniana) dynamicznie.

Po uruchomieniu sterownika poleceniem service up … wszystkie elementy bufora mają zostać wypełnione wielokrotnością ciągu kodów ASCII liter x, y, i z. Jeśli wielkość bufora nie jest podzielna przez 3, ostatnie wystąpienie ciągu powinno zostać odpowiednio skrócone.

Czytanie z urządzenia za pomocą funkcji read ma powodować odczytanie wskazanej liczby bajtów z początku kolejki. Gdy w kolejce nie znajduje się dostateczna liczba bajtów, należy odpowiednio zredukować wartość parametru polecenia czytania. Odczytane bajty usuwa się z kolejki. Kolejność odczytanych bajtów powinna odpowiadać kolejności, w jakiej bajty te były wkładane do kolejki. Gdy po odczytaniu wskazanego ciągu bajtów bufor będzie zajęty w co najwyżej jednej czwartej, należy zmniejszyć jego rozmiar o połowę. W przypadku wielkości nieparzystej, nowa rozmiar powinien zostać zaokrąglony w dół do wartości całkowitej.

Operacja pisania do urządzenia za pomocą funkcji write powoduje zapisanie wskazanego ciągu bajtów na końcu kolejki. Gdy rozmiar bufora nie wystarcza do zapisania wskazanego ciągu, należy pojemność bufora podwajać, aż będzie on wystarczająco duży na zapisanie całego ciągu.

Oprócz obsługi operacji czytania i pisania sterownik powinien implementować również operację ioctl pozwalającą na wykonanie następujących komend:

    HQIOCRES – Przywraca kolejkę do stanu początkowego – bufor powinien być rozmiaru DEVICE_SIZE oraz wypełniony wielokrotnością ciągu x, y, i z.
    HQIOCSET – Przyjmuje char[MSG_SIZE] i umieszcza przekazany napis w kolejce. Gdy rozmiar bufora jest nie mniejszy niż długość napisu, funkcja podmienia MSG_SIZE znaków z końca kolejki. Jeżeli napis zaś jest dłuższy od aktualnego rozmiaru bufora kolejki, to bufor powinien zostać powiększony tak samo jak podczas pisania do urządzenia, a następnie napis powinien zostać umieszczony w kolejce tak jak w pierwszym przypadku. Po operacji ostatni znak przekazanego napisu powinien znajdować się na końcu kolejki.
    HQIOCXCH – Przyjmuje char[2] i zamienia wszystkie wystąpienia w kolejce char[0] na char[1].
    HQIOCDEL – Usuwa co trzeci element z kolejki, zaczynając operację od początku kolejki (numerując elementy kolejki od 1, usuwamy elementy o numerach 3, 6, 9, …). Rozmiar bufora powinien pozostać bez zmian.

Stała MSG_SIZE jest zdefiniowana w dostarczanym przez nas pliku nagłówkowym ioc_hello_queue.h. Zastrzegamy możliwość zmiany wartości tej stałej w testach.

Wykonanie funkcji lseek nie powinno powodować zmian w działaniu urządzenia.

Ponadto urządzenie powinno zachowywać aktualny stan w przypadku przeprowadzenia jego aktualizacji poleceniem service update oraz w przypadku restartu poleceniem service restart.
