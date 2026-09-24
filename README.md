# LVGL-ESP32S3 — Zephyr, 128×128

Bu dal, eski ESP-IDF/FreeRTOS uygulamasını **Zephyr v4.1.0** ve onun
LVGL entegrasyonuna taşır. Arayüz **128×128** çözünürlüktedir.
`main` dalından bağımsızdır.

## Donanım hedefi

ESP32-S3-DevKitC için derleme hedefi:
`esp32s3_devkitc/esp32s3/procpu`.


| Sinyal | ESP32-S3 GPIO |
| --- | --- |
| SCK | 12 |
| MOSI | 11 |
| CS | 10 |
| DC | 9 |
| RESET | 8 |
| Besleme/GND | Modül özelliklerine uygun 3.3 V/GND |

Arka ışığı modülün gerektirdiği devreyle sür. Ekran çeşidine göre başlangıç
ofsetleri/renk sırası değişebilir; referans overlay `x-offset=2`,
`y-offset=3` kullanır. Farklı denetleyici veya bağlantı için
`boards/esp32s3_devkitc_esp32s3_procpu.overlay` dosyasını uyarlamak gerekir.

Dokunmatik denetleyici bilinmediğinden fiziksel dokunmatik sürücüsü
varsayılmamıştır. Yenileme UART üzerinden de çalışır. Zephyr input/LVGL
pointer desteğiyle kendi dokunmatik aygıtını ekleyebilirsin.

## Mimari
- LVGL'ye yalnızca ana iş parçacığı erişir
- Wi-Fi/DHCP ve yeniden bağlanma ayrı Zephyr work queue üzerinden yürür.
- HTTPS sorguları ayrı kernel thread içinde periyodik çalışır; tek istekten sonra bitmez.
- Parçalı HTTP gövdeleri 4096 baytlık sınırlı tamponda biriktirilir. Taşma,
  eksik yanıt, HTTP hata kodu ve bozuk JSON yeni ölçüm olarak gösterilmez.
- JSON şeması korunur: `things[0].device.SensorValue[0].value`.
- Sensör modeli mutex ile korunur; hata halinde son başarılı değer ve yaşı korunur.
- Yenileme isteği semaphore ile ağ thread'ine iletilir.
- Wi-Fi/API ayarları Zephyr settings/NVS'de saklanır; firmware içinde kullanıcı
  parolası veya API token'ı yoktur.
- TLS doğrulaması zorunludur: CA veya saat senkronizasyonu yoksa istek yapılmaz.
- 128×128 arayüzde değer, bağlantı/API durumu, ölçüm yaşı ve yenileme düğmesi bulunur.

Büyük LVGL 8 görsel/font çıktıları `assets/lvgl8/` altında referans olarak
korunur; Zephyr LVGL uygulamasına derlenmez. Yeni arayüz sabit 480×480
koordinatlarına bağımlı değildir.

## Derle ve yükle

Zephyr host bağımlılıkları, west ve ESP32-S3 toolchain'li Zephyr SDK 0.17.0 gerekir.

```sh
git clone --branch zephyr https://github.com/ugurakas/LVGL-ESP32S3.git
west init -l LVGL-ESP32S3
west update
west zephyr-export
python -m pip install -r zephyr/scripts/requirements-base.txt
west blobs fetch hal_espressif
west build -b esp32s3_devkitc/esp32s3/procpu LVGL-ESP32S3 -d build
west flash -d build
```

Bu temel derleme ekranı ve Wi-Fi yapılandırmasını çalıştırır. Gerçek HTTPS
sunucusunu kullanmak için sunucunun güvenilir **kök CA sertifikasını** yerel
bir PEM dosyası olarak temin edip derlemeye ver:

```sh
west build -p always -b esp32s3_devkitc/esp32s3/procpu LVGL-ESP32S3 -d build -- -DAPP_CA_CERT_FILE=/absolute/path/root-ca.pem
west flash -d build
```

CA bir özel anahtar değildir. Uygulama özel anahtar/token dosyası istemez.
CA verilmezse HTTPS `-EACCES` ile kapalı kalır; doğrulamasız bağlantıya düşmez.

115200-baud UART konsolunda gerçek değerlerinle yapılandır:

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


NVS ve yerel shell geliştirme amaçlıdır: flash şifrelenmez; konsol girişi
ekranda ve shell geçmişinde görülebilir. Gerçek kimlik bilgilerini Git'e ekleme.


## Testler

```sh
ZEPHYR_TOOLCHAIN_VARIANT=host west build -b native_sim/native/64 LVGL-ESP32S3/tests/app -d build-tests
SDL_VIDEODRIVER=dummy west build -d build-tests -t run
```

Testler parçalı HTTP gövdesi, tampon taşması, bozuk/yanlış tipli JSON,
hata sonrası son ölçümün korunması, 128×128 yerleşim ve yenileme olayını kapsar.
GitHub Actions ESP32-S3 firmware'ini ve simülatörü derler, bu testleri çalıştırır.

Donanım kabul testi: ekran renk/ofsetleri, UART provisioning, NVS'nin reboot
sonrası korunması, AP kesintisi sonrası bağlantı, gerçek API/TLS sertifikası,
yanlış CA reddi, büyük/parçalı yanıt ve son ölçümün hata halinde korunması.
