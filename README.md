# Mini Uydu OBC — Sıfırdan Kurulum Rehberi
### STM32F103C8T6 (Blue Pill) + BMP280 + MPU6050 + UART Telemetri — PICSimLab Simülasyonu

Bu rehber, bilgisayarınızda **hiçbir şey kurulu/açık olmadığını** varsayar ve sizi en
baştan alıp çalışan bir simülasyona kadar götürür.

---

## İÇİNDEKİLER

- [0) Gerekli Yazılımları İndirin ve Kurun](#0-gerekli-yazılımları-i̇ndirin-ve-kurun)
- [1) STM32CubeMX ile Proje Oluşturma](#1-stm32cubemx-ile-proje-oluşturma)
- [2) STM32CubeIDE'de Kod Ekleme ve Derleme](#2-stm32cubeidede-kod-ekleme-ve-derleme)
- [3) PICSimLab Kurulumu ve Devre Bağlantısı](#3-picsimlab-kurulumu-ve-devre-bağlantısı)
- [4) Simülasyonu Çalıştırma](#4-simülasyonu-çalıştırma)
- [5) (Opsiyonel) Python Yer İstasyonu ile Tam Test](#5-opsiyonel-python-yer-istasyonu-ile-tam-test)
- [6) Sorun Giderme](#6-sorun-giderme)

---

## 0) Gerekli Yazılımları İndirin ve Kurun

Aşağıdaki üç programı sırayla kurun (hepsi ücretsiz):

| # | Yazılım | İndirme adresi |
|---|---|---|
| 1 | **STM32CubeMX** | st.com üzerinden "STM32CubeMX" arayın → ST hesabıyla ücretsiz indirin |
| 2 | **STM32CubeIDE** | st.com üzerinden "STM32CubeIDE" arayın → ücretsiz indirin |
| 3 | **PICSimLab** | github.com/lcgamboa/picsimlab → "Releases" sekmesinden işletim sisteminize uygun kurulum dosyasını indirin |

Kurulum sırasını mutlaka bu şekilde takip edin: önce CubeMX, sonra CubeIDE (CubeIDE
kurulumu sırasında CubeMX'i de otomatik entegre eder, ayrıca kurmuş olmanız sorun
değildir), en son PICSimLab.

> Kurulumlar bilgisayarınıza göre 15-30 dakika sürebilir (özellikle CubeIDE büyük bir
> indirmedir). Hepsi bitene kadar bir sonraki adıma geçmeyin.

---

## 1) STM32CubeMX ile Proje Oluşturma

### 1.1 Yeni proje başlatın
1. STM32CubeMX'i açın.
2. Ana ekranda **"ACCESS TO MCU SELECTOR"** butonuna tıklayın (ya da `File > New Project`).
3. Arama kutusuna `STM32F103C8` yazın, listeden **STM32F103C8Tx** paketini seçin.
4. Sağ üstteki **"Start Project"** butonuna basın.

### 1.2 Saat kaynağı (RCC)
Sol taraftaki `System Core > RCC` sekmesine tıklayın:
- **High Speed Clock (HSE):** `Crystal/Ceramic Resonator` seçin.

### 1.3 Sistem ayarları (SYS)
`System Core > SYS` sekmesine tıklayın:
- **Debug:** `Serial Wire` seçin.
- **Timebase Source:** `SysTick` (genelde varsayılan).

### 1.4 I2C1 (sensörler için)
`Connectivity > I2C1` sekmesine tıklayın:
- **Mode:** `I2C` işaretleyin.
- Bu otomatik olarak Pinout görünümünde **PB6 = I2C1_SCL**, **PB7 = I2C1_SDA** atar.
- `Parameters` sekmesinde: **I2C Speed Mode = Standard Mode**, **I2C Clock Speed = 100000**.

### 1.5 USART1 (haberleşme/telemetri için)
`Connectivity > USART1` sekmesine tıklayın:
- **Mode:** `Asynchronous`
- Bu otomatik olarak **PA9 = USART1_TX**, **PA10 = USART1_RX** atar.
- `Parameters` sekmesinde: **Baud Rate = 115200**, geri kalanlar varsayılan (8 bit, Parity None, 1 stop bit).

### 1.6 PC13 — dahili LED (opsiyonel ama önerilir)
Pinout görünümünde kart resmi üzerinde **PC13** pinine tıklayın → **GPIO_Output** seçin.
(Bu, kartın üzerindeki LED'i "hâlâ çalışıyorum" göstergesi olarak yanıp söndürmek için.)

### 1.7 Saat ağacını (Clock Tree) ayarlayın
Üstteki **"Clock Configuration"** sekmesine geçin:
1. HSE kutusundan gelen oku takip edip **PLL Source Mux**'ı `HSE` yapın.
2. **PLL Mul** değerini `X9` yapın.
3. **System Clock Mux**'ı `PLLCLK` yapın.
4. Sağ üstte kırmızı/sarı renkli kutucuklar (uyarı) görürseniz, üstteki
   **"Resolve Clock Issues"** butonuna basın — CubeMX otomatik olarak doğru
   APB1/APB2 önbölücülerini (prescaler) ayarlar (APB1=36MHz, APB2=72MHz, SYSCLK=72MHz olmalı).

### 1.8 Proje ayarları ve kod üretimi
**"Project Manager"** sekmesine geçin:
- **Project Name:** İstediğiniz bir isim (örn. `MiniUyduOBC`)
- **Project Location:** Kod dosyalarının kaydedileceği klasör
- **Toolchain / IDE:** `STM32CubeIDE`

Sağ üstteki **"GENERATE CODE"** butonuna basın. İşlem bitince çıkan pencerede
**"Open Project"** butonuna basarak STM32CubeIDE'yi açın (otomatik açılmazsa,
CubeIDE'yi elle açıp projeyi `File > Import > Existing Projects into Workspace`
ile içe aktarın).

---

## 2) STM32CubeIDE'de Kod Ekleme ve Derleme

STM32CubeIDE açıldığında, sol taraftaki **Project Explorer**'da proje adınızı göreceksiniz.

### 2.1 Sürücü dosyalarını ekleyin

Aşağıdaki 6 dosyayı bu mesajın sonunda indirebileceksiniz. Şu klasörlere kopyalayın:

| Dosya | Hedef klasör |
|---|---|
| `bmp280.h` | `<proje>/Core/Inc/` |
| `bmp280.c` | `<proje>/Core/Src/` |
| `mpu6050.h` | `<proje>/Core/Inc/` |
| `mpu6050.c` | `<proje>/Core/Src/` |
| `telemetry.h` | `<proje>/Core/Inc/` |
| `telemetry.c` | `<proje>/Core/Src/` |

Dosyaları kopyaladıktan sonra, CubeIDE'de proje adına sağ tıklayıp **Refresh (F5)**
yapın ki yeni dosyalar Project Explorer'da görünsün.

> Windows Gezgini / Dosya Yöneticisi ile doğrudan proje klasörüne kopyalayabilirsiniz,
> ya da CubeIDE içinde `Core/Inc` klasörüne sağ tık → `Import` → `File System` ile de ekleyebilirsiniz.

### 2.2 `main.c`'yi düzenleyin

Sol taraftan `Core/Src/main.c` dosyasını açın. İçinde şuna benzer etiketler göreceksiniz:

```c
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */
```

**main_TAM_ENTEGRASYON.c** dosyasındaki (mesajın sonunda) her bloğu, `main.c`'deki
**aynı isimli** bloğun arasına yapıştırın. Toplam 5 blok var:

1. `Includes` → `#include` satırları
2. `PV` (Private Variables) → `bmp280_data`, `mpu6050_data`, `tlm_payload` değişkenleri
3. `PFP` + `0` → `__io_putchar` fonksiyonu (printf'i UART'a bağlar)
4. `2` → Sensörleri başlatma kodu (bu, `while(1)`'den hemen ÖNCE gelir)
5. `WHILE` → Ana döngü içeriği

> **Dikkat:** `main.c`'de zaten `while (1) { ... }` bloğu var. Biz sadece içini
> `USER CODE BEGIN WHILE` ile `USER CODE END WHILE` arasına ekliyoruz, döngünün
> kendisini silmiyoruz. Kaynak dosyadaki yorum satırlarını (`/* USER CODE BEGIN 3 */`
> gibi) olduğu gibi bırakın, sadece aralarına kod ekleyin.

### 2.3 `libm` (matematik kütüphanesi) bağlantısını ayarlayın

BMP280 sürücüsü yükseklik hesabı için `pow()` fonksiyonunu kullanır, bu yüzden linker'a
matematik kütüphanesini eklemeniz gerekir:

1. Project Explorer'da proje adına sağ tıklayın → **Properties**
2. `C/C++ Build > Settings > MCU GCC Linker > Libraries`
3. **Libraries (-l)** listesine **`m`** ekleyin (tek harf, başında `lib` sonunda uzantı
   OLMADAN — `libm` yazarsanız derleme hatası alırsınız).
4. **Apply and Close**.

### 2.4 Float `printf` desteğini açın (önerilir)

Aynı Properties penceresinde:
`C/C++ Build > Settings > MCU Settings` → **"Use float with printf from newlib-nano"**
kutusunu işaretleyin. Bu olmadan `%f` ile basılan ondalıklı sayılar (sıcaklık, basınç
vb.) doğru görünmeyebilir.

### 2.5 Derleyin

Üst menüden `Project > Build All` (ya da çekiç ikonuna tıklayın, ya da `Ctrl+B`).

Derleme hatasız biterse, sol taraftaki `Debug` (veya `Release`) klasöründe projenizin
adıyla biten bir **`.bin`** dosyası oluşur (örn. `MiniUyduOBC.bin`). Bunu Windows
Gezgini'nde göremiyorsanız proje klasörüne sağ tık → Refresh yapın.

---

## 3) PICSimLab Kurulumu ve Devre Bağlantısı

### 3.1 PICSimLab'i açın ve kartı seçin
1. PICSimLab'i başlatın.
2. Üstteki board seçim kutusundan **Blue_Pill** kartını seçin.
3. Processor olarak **stm32f103c8t6** seçili olduğundan emin olun.

### 3.2 Sensör parçalarını ekleyin
Üst menüden `Modules > Spare parts` yolunu izleyin, Spare Parts penceresi açılır.

**BMP280 ekleyin:**
1. `Inputs > BMP280 (Pressure I2C)` seçin, parça board üzerinde belirir.
2. Parçaya sağ tıklayıp **Properties** açın, şu şekilde ayarlayın:

   | Pin | Değer |
   |---|---|
   | SCL | **PB6** (board üzerinde "SCL" etiketli pin) |
   | SDA | **PB7** (board üzerinde "SDA" etiketli pin) |
   | CSB | **VDD** |
   | SDO | **VSS** |

**MPU6050 ekleyin:**
1. `Inputs > MPU6050` seçin.
2. Properties'ten:

   | Pin | Değer |
   |---|---|
   | SCL | **PB6** (aynı I2C hattı — BMP280 ile paylaşılır) |
   | SDA | **PB7** |
   | AD0 | **VSS** (GND) → adres 0x68, koddaki varsayılanla eşleşir |

   > İki sensör de aynı SCL/SDA hattına bağlanır — I2C, farklı adresli birden fazla
   > cihazı aynı hatta desteklemek üzere tasarlanmıştır. BMP280 adresi 0x76,
   > MPU6050 adresi 0x68 olduğu için çakışma olmaz.

**Terminal (debug/telemetri çıktısını görmek için) ekleyin:**
1. `Virtual > IO Virtual Term` (veya `Others` altında olabilir, sürüme göre değişir) seçin.
2. Properties'ten:

   | Pin | Değer |
   |---|---|
   | RX | **PA9** |
   | TX | **PA10** |
   | Speed | **115200** |

---

## 4) Simülasyonu Çalıştırma

1. PICSimLab ana penceresinde `File > Load Firmware` (veya araç çubuğundaki "yükle"
   ikonu) ile CubeIDE'nin ürettiği **.bin** dosyasını seçin.
2. **Run/Play** (▶) butonuna basın.
3. Az önce eklediğiniz **IO Virtual Term** penceresini açık tutun (kapalıysa
   Spare Parts listesinden tekrar çift tıklayarak açabilirsiniz).

### Beklenen çıktı:

```
========================================
 Mini Uydu OBC - Baslatiliyor
========================================
[BMP280] ChipID-status=0 ChipID=0x58 (beklenen 0x58) -> OK
[MPU6050] WhoAmI-status=0 WhoAmI=0x68 (beklenen 0x68) -> OK
[TLM] Telemetri modulu hazir (USART1, 115200 baud).
========================================
Tum sistemler hazir, ana donguye giriliyor...

[HEX] AA 55 00 2C 00 00 00 00 ... XX
[TLM #1] T=24.5C P=1013.2hPa Alt=0.7m | Ax=0.01 Ay=-0.02 Az=1.00 g | Gx=0.3 Gy=-0.1 Gz=0.0 dps
[HEX] AA 55 01 2C 00 00 00 00 ... XX
[TLM #2] T=24.5C P=1013.1hPa Alt=0.9m | Ax=0.01 Ay=-0.02 Az=1.00 g | Gx=0.2 Gy=-0.1 Gz=0.1 dps
...
```

PC13 LED'i her yarım saniyede bir yanıp söner. BMP280 ve MPU6050 parçalarının
üzerindeki kaydırıcılarla (sliderlar) basınç/sıcaklık/ivme değerlerini elle
değiştirerek okumaların canlı güncellendiğini gözlemleyebilirsiniz.

**Buraya kadar geldiyseniz sisteminiz baştan sona çalışıyor demektir.** 🎉

---

## 5) (Opsiyonel) Python Yer İstasyonu ile Tam Test

Yukarıdaki adım zaten sistemin çalıştığını kanıtlıyor. Eğer gerçek bir yer istasyonu
gibi, paketleri ayrıştırıp kayıp/checksum kontrolü yapan bir araçla test etmek
isterseniz:

1. Windows için **com0com**, Linux için **tty0tty** kurup sanal bir seri port çifti
   oluşturun (COM1↔COM2 gibi).
2. PICSimLab'in gerçek seri port ayarını (durum çubuğundaki port ismine tıklayarak
   veya Config penceresinden) bu çiftin bir ucuna (örn. COM1) bağlayın.
3. `pip install pyserial` ile pyserial kurun.
4. `python ground_station.py COM2` (çiftin diğer ucu) komutunu çalıştırın.

Bu adımı istediğiniz zaman, ayrı bir mesajda daha detaylı isteyebilirsiniz —
şimdilik 4. adımdaki simülasyon tek başına sisteminizin doğru çalıştığını göstermeye yeterlidir.

---
```
1
pyserial'i kurun
Bir terminal/komut istemi açın ve 'pip install pyserial' yazıp Enter'a basın. Kurulum bitene kadar bekleyin.
2
com0com'u indirip kurun (Windows)
https://sourceforge.net/projects/com0com/ adresine gidin, 'Files' sekmesinden en güncel sürümü (setup dosyası, imzalı/signed olanı) indirin. İndirdiğiniz .exe dosyasını çalıştırıp kurulum sihirbazını varsayılan seçeneklerle tamamlayın. Windows 'imzasız sürücu' uyarısı verirse 'Yükle' / 'Install this driver software anyway' seçin.
3
COM1 ↔ COM2 çiftini oluşturun
Kurulum bitince Başlat menüsünden 'Setup Command Prompt' (com0com ile birlikte gelir) ya da masaüstündeki 'com0com setup' kısayolunu açın. Açılan pencerede iki port çifti göreceksiniz (genelde CNCA0 ↔ CNCB0). Bunları tanıdık isimlere çevirmek için sırayla: 'change CNCA0 PortName=COM1' yazıp Enter, sonra 'change CNCB0 PortName=COM2' yazıp Enter'a basın. Ardından 'list' yazarak COM1 ve COM2'nin oluştuğunu doğrulayın. Pencereyi kapatabilirsiniz.
4
IO Virtual Term parçasını kaldırın
PICSimLab'de Spare Parts penceresinden mevcut IO Virtual Term parçasını seçip silin (sağ tık > Delete/Remove, ya da parçayı seçip Delete tuşuna basın). Böylece PA9/PA10 pinleri boşa çıkar.
5
PICSimLab'de gerçek seri portu COM1 olarak ayarlayın
PICSimLab ana penceresinin alt kısmındaki durum çubuğunda (status bar) seri port bilgisinin yazdığı alana tıklayın (bazı sürümlerde bu bir menü öğesi olarak da 'Serial' / 'Comm' adıyla üst menüde bulunur). Açılan ayardan portu COM1 olarak seçin ve hızın 115200 olduğundan emin olun. Bu, kartın PA9(TX)/PA10(RX) pinlerini doğrudan COM1'e bağlar.
6
ground_station.py'yi COM2 üzerinden çalıştırın
'main_TAM_ENTEGRASYON.c' ile birlikte indirdiğiniz ground_station.py dosyasının bulunduğu klasöre terminalde gidin (cd komutuyla). 'python ground_station.py COM2' yazıp çalıştırın. Bu, com0com çiftinin COM1'e KARŞI ucudur — PICSimLab COM1'e yazar, siz COM2'den okursunuz.
7
PICSimLab'de Run'a basıp Python çıktısını izleyin
PICSimLab'de Load Firmware ile .bin dosyanızı zaten yüklediyseniz sadece Run (▶) butonuna basın (önce yeniden yüklemeniz gerekmez, kod değişmedi). Python terminalinde gerçek zamanlı, checksum'ı doğrulanmış, okunaklı telemetri satırlarının akmaya başladığını göreceksiniz.
```
## 6) Sorun Giderme

| Belirti | Çözüm |
|---|---|
| `cannot find -llibm` derleme hatası | Linker Libraries listesinde `libm` yerine sadece `m` yazın (bkz. 2.3) |
| `[BMP280] ... -> HATA`, ChipID ≠ 0x58 | BMP280 parçasının SCL/SDA'sının PB6/PB7'ye bağlı olduğunu, CSB=VDD, SDO=VSS olduğunu kontrol edin |
| `[MPU6050] ... -> HATA`, WhoAmI ≠ 0x68 | MPU6050 parçasının SCL/SDA'sının PB6/PB7'ye bağlı olduğunu, AD0=VSS olduğunu kontrol edin |
| IO Virtual Term'de hiçbir şey görünmüyor | RX=PA9, TX=PA10, Speed=115200 ayarlarını kontrol edin; pencerenin açık olduğundan emin olun |
| `%f` ile ondalık sayılar yanlış basılıyor | "Use float with printf from newlib-nano" seçeneğini işaretleyin (bkz. 2.4) |
| Basınç/ivme değerleri hep aynı | Normal — parçaların üzerindeki kaydırıcılarla elle değiştirin |
| Derleme sonrası `.bin` dosyası yok | Proje Properties → `C/C++ Build > Settings > MCU Post build outputs` → "Convert to binary (.bin)" kutusunu işaretleyin, tekrar derleyin |
