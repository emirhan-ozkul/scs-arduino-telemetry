# ETS2 / ATS Telemetry — Kurulum ve Kullanım Rehberi

Bu rehber Windows bilgisayarda uygulamanın kurulumu, ETS2/ATS native plugin'in yüklenmesi, Arduino'nun bağlanması ve sistemin çalıştırılması için hazırlanmıştır.

## 1. Önkoşullar

### Bilgisayar

- Windows 10 veya Windows 11
- USB veya dahili seri port bağlantısı
- ETS2 veya ATS için oyuna erişim
- Uyumlu bir Arduino:
  - Arduino Uno
  - Arduino Leonardo
  - Arduino Nano

### Gerekli Yazılımlar

Portatif sürüm için Python kurulması gerekli değildir.

Kaynak koddan çalıştırma veya geliştirme için:

- Python 3.11 veya yeni sürüm
- `pyserial`
- Tkinter
- İsteğe bağlı olarak Arduino IDE veya Arduino CLI

Native plugin'i yeniden derlemek için:

- Visual Studio 2022 C++ araçları
- Windows SDK
- CMake 3.20 veya yeni sürüm

## 2. Portatif Paketi Çalıştırma

### Paket dosyalarını ayıklama

`dist/ETS2Telemetry-Release.zip` dosyasını klasöre ayıklayın veya mevcut `dist/ETS2TelemetryPortable` klasörünü kullanın.

Önerilen klasör:

```text
C:\ETS2Telemetry\
```

Ana dosya:

```text
C:\ETS2Telemetry\ETS2Telemetry.exe
```

Paket doğrudan bu exe dosyası çalıştırılarak açılır. Python kurulması gerekmiyor.

### İlk çalıştırma

1. `ETS2Telemetry.exe` dosyasını çift tıklayın.
2. “ETS2 / ATS Telemetry” penceresi açılmalı.
3. COM port seçimi yapılmalı.
4. UDP portu varsayılan olarak `12345` kullanılmalı.
5. Baud rate varsayılan olarak `115200` kullanılmalı.
6. **Start** düğmesine basılmalı.

GUI açılırsa PyInstaller paketinin Windows çalışma zamanı testi geçmiş demektir.

## 3. Native SCS Plugin'in Kurulumu

Native DLL dosyası portatif pakette şunlardan bulunur:

```text
ETS2TelemetryPlugin.dll
```

Paketli DLL'nin mimarisinin x64 olduğu doğrulanmıştır.

### 3.1 Oyun executable mimarisini belirleme

Oyun executable dosyanızın bulunduğu klasörde PowerShell açın ve örneğin:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\SDK\ScopeCppSDK\vc15\VC\bin\dumpbin.exe" /headers "<OYUN_KLASORU>\bin\win_x64\eurotrucks2.exe"
```

Çıktıda şu ifadeyi arayın:

```text
machine (x64)
```

32-bit executable'da `machine (x86)` görünmelidir.

x64 DLL'yi 32-bit oyuna veya x86 DLL'yi 64-bit oyuna kurmayın.

### 3.2 Plugin klasörünü doğrulama

SCS SDK 1.16, plugin'in executable'a göre oyun binary klasörünün altındaki `plugins` dizininde bulunabileceğini belirtir:

```text
bin\win_x64\plugins
```

Ancak Steam veya özel kurulum klasörüne göre gerçek yol değişebilir. DLL'yi ilk önce otomatik olarak kopyalamayın; `plugins` klasörünü gerçek oyun kurulumunda doğrulayın.

### 3.3 DLL kurulum yöntemi

Yalnızca gerçek executable mimarisi doğrulandıktan sonra DLL'yi oyun `plugins` klasörüne manuel kopyalayın:

```text
ETS2TelemetryPlugin.dll
```

```text
<OYUN_KLASORU>\bin\win_x64\plugins\ETS2TelemetryPlugin.dll
```

Oyun başlatılırken plugin exportları aranır:

- `scs_telemetry_init`
- `scs_telemetry_shutdown`

### 3.4 Plugin yüklemesini doğrulama

1. ETS2 veya ATS oyununu başlatın.
2. Bir sürüş oturumuna girin.
3. Oyun telemetri log/plugin logunu inceleyin.
4. Plugin log dosyasını açın:

```text
%TEMP%\ETS2TelemetryPlugin\telemetry_plugin.log
```

Beklenen log türleri:

- `Initializing SCS telemetry API 1.01`
- `Detected ETS2 game` veya `Detected ATS game`
- Her channel için `Channel ... registration`
- `UDP telemetry target: 127.0.0.1:12345`
- `Telemetry state: started/driving`

### 3.5 SDK 1.62 notu

SDK 1.16 içindeki game-version declarations ETS2/ATS 1.61'e kadar bilgi içerir. Bu, DLL'nin 1.61 oyununda gerçek runtime uyumluluğunu kanıtlamaz.

ETS2 1.61 ve ATS 1.61 live testleri elle yapılmalıdır. Başarılı yüklenme ve veri akışı kanıtlanmadan uyumluluk PASS olarak işaretlenmemelidir.

## 4. Arduino Kurulumu

### 4.1 Arduino IDE ile firmware yükleme

1. Arduino IDE'yi açın.
2. `arduino/telemetry_receiver/telemetry_receiver.ino` dosyasını açın.
3. Board seçin:
   - Uno için `Arduino Uno`
   - Leonardo için `Arduino Leonardo`
   - Nano için `Arduino Nano`
4. Doğru COM portunu seçin.
5. `Verify` ve ardından `Upload` işlemlerini çalıştırın.

### 4.2 Arduino CLI ile derleme

Uno:

```powershell
& "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn arduino:avr:uno arduino\telemetry_receiver
```

Leonardo:

```powershell
& "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn arduino:avr:leonardo arduino\telemetry_receiver
```

Nano:

```powershell
& "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile --fqbn arduino:avr:nano arduino\telemetry_receiver
```

Not: Arduino firmware fiziksel LCD veya LED sürücüsü eklemez; minimum sürüm seri protokolü, doğrulama ve timeout mantığını uygular.

## 5. Windows GUI Kullanımı

### UDP ayarı

- `UDP host`: varsayılan `127.0.0.1`
- `UDP port`: varsayılan `12345`

UDP portu, native plugin ve Python GUI ile aynı olmalıdır. Native plugin portunu ortam değişkeni ile değiştirmek mümkündür:

```powershell
$env:ETS2_TELEMETRY_UDP_PORT="12345"
```

Bu değişken, native plugin DLL'nin yüklendiği süreç için oyunu başlatmadan önce ayarlanmalıdır.

GUI'de port değiştirildikten sonra **Start** düğmesine basın.

### COM port seçimi

1. Arduino'yu USB üzerinden bağlayın.
2. GUI'de **Refresh COM** düğmesine basın.
3. Açılan COM listesinde Arduino portunu seçin.
4. Baud rate olarak `115200` kullanın.
5. **Start** düğmesine basın.

GUI serial bağlantıları kaybederse otomatik olarak yeniden bağlanmayı dener.

### Start ve Stop

- **Start**: UDP listener ve serial worker'ları başlatır.
- **Stop**: UDP ve COM bağlantılarını durdurur.
- Pencere kapatılırsa service otomatik olarak durdurulur.

## 6. Tam Sistem Testi

1. Native DLL'yi doğrulanmış plugin dizinine yükleyin.
2. Arduino'yu 115200 baud firmware ile yükleyin.
3. Arduino'yu USB COM portuna bağlayın.
4. GUI'deki COM portunu seçin.
5. GUI'de **Start** düğmesine basın.
6. ETS2 veya ATS'i başlatın.
7. Bir sürüş oturumuna girin.
8. Oyunda hızlanın veya gaza basın.
9. GUI'de hız, RPM ve gear alanlarının güncellendiğini doğrulayın.
10. Plugin ve Python UDP paketi gelmiyorsa stale durumunun açıkça gösterildiğini kontrol edin.

Beklenen telemetri oyun verileri:

- Vehicle speed
- Engine RPM
- Gear
- Fuel amount
- Navigation remaining distance
- Odometer
- Average fuel consumption
- Water/engine temperature

## 7. Kaynak Koddan Çalıştırma

Python bağımlılıklarını yükleyin:

```powershell
python -m pip install -r python\requirements.txt
```

Testleri çalıştırın:

```powershell
python -m unittest discover -s tests -p "test_*.py" -v
```

GUI'yi çalıştırın:

```powershell
python python\main.py
```

## 8. Portatif Paketi Yeniden Oluşturma

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package.ps1
```

Oluşturulan çıktı:

```text
dist\ETS2Telemetry-Release.zip
```

Gerçek ZIP yolunun `dist/ETS2Telemetry-Release.zip` olduğunu doğrulayın. Script sürümünde son package ZIP adı `ETS2Telemetry-$Configuration.zip` biçimindedir.

## 9. Sorun Giderme

### GUI açılmıyor

- `ETS2Telemetry.exe` ile birlikte `_internal` klasörünün bulunduğunu kontrol edin.
- Exe dosyasını tek başına taşıyan değil, tüm `ETS2TelemetryPortable` klasörünü taşıyın.
- `ETS2Telemetry.exe` dosyasının Windows Defender veya üçüncü taraf antivirüs tarafından engellenmediğini kontrol edin.

### COM port görünmüyor

- Arduino USB kablosunu çıkarıp takın.
- **Refresh COM** düğmesine basın.
- Arduino IDE veya Windows Device Manager'da COM portunu doğrulayın.
- Yanlış sürücüyü kaldırmadan Nano/Uno/Leonardo board bilgisini kontrol edin.

### COM bağlantısı kopuyor

- Baud değerinin `115200` olduğunu doğrulayın.
- Başka bir programın COM portunu kullanmadığını kontrol edin.
- USB kablosunun veri kablosu olduğundan emin olun.

### UDP veri gelmiyor

- GUI UDP portu ile native plugin UDP portunun eşit olduğunu kontrol edin.
- `ETS2_TELEMETRY_UDP_PORT` ortam değişkenini DLL yüklemeden önce ayarlayın.
- Windows güvenlik duvarının localhost UDP trafiğini engellemediğini kontrol edin.
- Plugin logunda channel registration hatalarını kontrol edin.

### Plugin yüklenmiyor

- DLL mimarisini oyun executable mimarisiyle karşılaştırın.
- DLL'nin gerçek plugin klasöründe olduğunu doğrulayın.
- Plugin exportlarını kontrol edin:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\SDK\ScopeCppSDK\vc15\VC\bin\dumpbin.exe" /exports "C:\ETS2Telemetry\ETS2TelemetryPlugin.dll"
```

### Kanallar unavailable

Channel callback'leri oyun tarafından bulunamıyorsa logda `SCS_RESULT_not_found` veya `SCS_RESULT_unsupported_type` görünür. Kullanılabilir olmayan alanlar GUI'de `Unavailable`, Arduino protokolünde `NA` olarak gösterilir.

## 10. Güvenlik ve Bilinen Sınırlamalar

- DLL otomatik olarak oyun klasörüne kopyalanmaz.
- ETS2 1.61 ve ATS 1.61 live runtime uyumluluğu doğrulanmamıştır.
- Arduino fiziksel ekran/LED kontrolü bu minimum sürümde donanım bağımlı olarak uygulanmamıştır.
- COM port ve USB donanım testleri kullanıcı tarafından yapılmalıdır.
- Firmware yalnızca AVR derleme testinden geçirilmiştir; fiziksel Arduino testleri henüz yapılmamıştır.
