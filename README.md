# LVGL-ESP32S3

ESP32-S3 üzerinde çalışan, bir API'den aldığı sensör değerini 128×128 piksel ekranda gösteren küçük bir panel uygulaması. Ekranda son ölçümün yanında Wi-Fi ve API durumu, ölçümün ne kadar önce alındığı ve yenileme düğmesi bulunuyor.

Proje, önceki ESP-IDF/FreeRTOS sürümünden Zephyr v4.1.0'a taşındı. Arayüz için Zephyr'in LVGL entegrasyonu kullanılıyor.

## Donanım ve bağlantılar

Derleme hedefi **ESP32-S3-DevKitC**. Depodaki örnek ekran yapılandırması, SPI üzerinden bağlanan **128×128 ST7735R** için hazırlanmış.

| Ekran sinyali | ESP32-S3 GPIO |
| --- | --- |
| SCK | 12 |
| MOSI | 11 |
| CS | 10 |
| DC | 9 |
| RESET | 8 |
| Besleme / GND | Modüle uygun 3.3 V / GND |

Bu bağlantılar bir referans; kullanmadan önce ekranının denetleyicisini ve pinlerini kontrol et. Arka ışık bağlantısını da modülün gerektirdiği şekilde yap.

Ekran ayarları [board overlay dosyasında](boards/esp32s3_devkitc_esp32s3_procpu.overlay) bulunuyor. Mevcut yapılandırmada `x-offset=2` ve `y-offset=3` kullanılıyor. Görüntü kayıyorsa veya renkler yanlışsa ofsetleri ve renk sırasını ekranına göre düzenle.

Fiziksel dokunmatik sürücüsü henüz tanımlı değil. Dokunmatik eklemek için Zephyr input ve LVGL pointer desteği kullanılabilir. Ölçümü UART konsolundan da yenileyebilirsin.

## Kurulum

Önce Zephyr'in bilgisayar tarafındaki bağımlılıklarını, `west` aracını ve ESP32-S3 araç zincirini içeren **Zephyr SDK 0.17.0** sürümünü kur.

Aşağıdaki komutları projeyi tutmak istediğin çalışma klasöründe çalıştır:

```sh
git clone https://github.com/ugurakas/LVGL-ESP32S3.git
west init -l LVGL-ESP32S3
west update
west zephyr-export
python -m pip install -r zephyr/scripts/requirements-base.txt
west blobs fetch hal_espressif
west build -b esp32s3_devkitc/esp32s3/procpu LVGL-ESP32S3 -d build
west flash -d build
```

Bu derleme ekranı ve Wi-Fi yapılandırmasını çalıştırmak için yeterli. API'ye HTTPS üzerinden bağlanmak için sunucunun güvenilir kök CA sertifikasını da derlemeye eklemek gerekiyor.

### HTTPS sertifikası

Kök CA sertifikasını PEM dosyası olarak kaydet ve dosyanın tam yolunu vererek yeniden derle:

```sh
west build -p always -b esp32s3_devkitc/esp32s3/procpu LVGL-ESP32S3 -d build -- -DAPP_CA_CERT_FILE=/absolute/path/root-ca.pem
west flash -d build
```

Burada gereken dosya, sunucuya güvenmek için kullanılan CA sertifikasıdır; özel anahtar değildir. CA verilmezse HTTPS istekleri `-EACCES` hatasıyla durur. Uygulama sertifika doğrulamasını kapatarak devam etmez. Bağlantı için saat senkronizasyonunun da tamamlanması gerekir.

### Wi-Fi ve API ayarları

UART konsolunu **115200 baud** hızında aç. Aşağıdaki yer tutucuları kendi bilgilerinle değiştir:

```text
panel set ssid "<ssid>"
panel set password "<wifi-password>"
panel set api_host "<api-hostname>"
panel set api_path "/Thing/GetThingsWithDevice"
panel set api_body '<your-json-request-body>'
panel set ntp_host "pool.ntp.org"
panel reboot
panel refresh
```

Ayarlar Zephyr settings/NVS üzerinden saklanır ve yeniden başlatıldığında korunur. Wi-Fi parolası ve API için gereken bilgiler kaynak koda gömülmez.

Bu ayar yöntemi geliştirme içindir: flash şifrelenmez, konsola yazılan bilgiler ekranda ve komut geçmişinde görünebilir. Gerçek parolaları veya API bilgilerini Git'e ekleme.

## Nasıl çalışıyor?

Uygulama Wi-Fi'ye bağlandıktan sonra API'yi düzenli aralıklarla sorgular. Bağlantı kesilirse yeniden bağlanmayı dener. Yenileme düğmesi veya `panel refresh` komutu da yeni bir sorgu başlatır.

Yanıttan okunan sensör alanı:

```text
things[0].device.SensorValue[0].value
```

Parçalar halinde gelen HTTP yanıtları, en fazla 4096 baytlık bir tamponda birleştirilir. Yanıt eksikse, tampona sığmıyorsa, HTTP hatası içeriyorsa veya JSON geçersizse yeni ölçüm olarak gösterilmez. Ekranda son başarılı ölçüm ve ne kadar önce alındığı kalır.

Kod tarafında ekranı ana iş parçacığı günceller; LVGL çağrıları yalnızca buradan yapılır. Wi-Fi işlemleri ayrı bir iş kuyruğunda, HTTPS sorguları ayrı bir iş parçacığında çalışır. Ortak sensör verisi mutex ile korunur, yenileme isteği ise semaphore üzerinden ağ tarafına iletilir.

Önceki LVGL 8 arayüzünün görsel ve font dosyaları [assets/lvgl8](assets/lvgl8/) altında duruyor. Bunlar referans olarak saklanıyor ve mevcut uygulamaya derlenmiyor. Yeni arayüz 128×128 ekran için hazırlanmış durumda.

## Testler

Simülatör testlerini Linux ortamında şu komutlarla çalıştırabilirsin:

```sh
ZEPHYR_TOOLCHAIN_VARIANT=host west build -b native_sim/native/64 LVGL-ESP32S3/tests/app -d build-tests
xvfb-run -a west build -d build-tests -t run
```

Testler; parçalı HTTP yanıtlarını, tampon taşmasını, bozuk veya beklenmeyen tipteki JSON verilerini, hata sonrası son ölçümün korunmasını, 128×128 ekran yerleşimini ve yenileme olayını kapsıyor. GitHub Actions da ESP32-S3 firmware'ini ve simülatörü derleyip testleri çalıştırıyor.

Gerçek kart üzerinde ekran renkleri ve ofsetleri, dokunmatik, Wi-Fi kesintisinden sonra yeniden bağlanma ve API/TLS bağlantısı ayrıca denenmeli. Ayarların yeniden başlatma sonrası korunması, yanlış CA sertifikasının reddedilmesi ve hatalı yanıt geldiğinde son ölçümün ekranda kalması da donanım kontrolünün bir parçası.
