# tft_platformio_v1
Oprogramowanie wyswietlacza w srodowisku platform.io

Potrzebne do dzialania:
- Visual Studio Code z rozszerzeniem platform.io
- plytka obslugiwana przez mbed, posiadajaca SPI oraz CANa, tutaj uzyta 446RE
- wyswietlacz TFT z ST7335: https://www.waveshare.com/1.8inch-lcd-module.htm
- wszystkie biblioteki zostaly zedytowane pod dany wyswietlacz i są już w repozytorium, nie trzeba nic dodatkowo sciagac

Konfiguracja:
- jezeli bedzie uzyta inna plytka to nalezy zedytowac plik "platformio.ini" gdzie nalezy zmienic model lub utworzyc nowy projekt, w     kreatorze wybrac dana plytke i zamienic te pliki
- w pliku "Adafruit_ST7735.cpp" pod funkcja "void Adafruit_ST7735::initR" (259 wiersz) znajduja sie rozne opcje inicjalizacji w zaleznosci od wyswietlacza, jezeli zmienia sie wyswietlacz trzeba podac oraz offset ekranu - "colstart" i "rowstart"
- predkosc SPI mozna ustawic w pliku "Adafruit_ST7735.cpp" (234 wiersz) - "lcdPort.frequency(40000000);"
- parametry CANa mozna zmienic w pliku "main.ccp" (76 wiersz)

Pojawiajace sie po kompilacji ostrzezenia nie wplywaja na dzialanie programu, natomiast nalezy sprawdzic czy jest mozliwosc ich wyeliminacji.
