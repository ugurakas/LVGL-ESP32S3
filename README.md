# LVGL-ESP32S3

LVGL tabanlı, modüler UI mimarisine sahip, ESP32-S3 üzerinde ESP-IDF ile çalışan bir grafik arayüz projesi. 128×128 dokunmatik ekranlı bir cihazda WiFi taraması yapar, bulunan ağa otomatik bağlanır, bir REST API'den sensör verisi çeker ve ekranda gösterir.

## Özellikler

- **WiFi yönetimi:** Kapsama alanındaki ağları tarar, yapılandırılan SSID'ye otomatik bağlanır, bağlantı koptuğunda yeniden dener (maksimum deneme sayısı ayarlanabilir).
- **LVGL tabanlı arayüz:** Qt Design benzeri bir araçla oluşturulmuş görsel arayüzler, modüler ekran/widget yapısı.
- **Eşzamanlılık güvenliği:** Ekran geçişlerinde ve arayüz güncellemelerinde çakışmaları önlemek için semaphore/mutex kullanımı.
- **REST API entegrasyonu:** HTTPS üzerinden POST isteğiyle sensör verisi çeker, JSON yanıtı ayrıştırıp ekrana yansıtır.
- **Genişletilebilir:** MQTT veya farklı bir HTTP endpoint'i ile kolayca değiştirilebilecek şekilde tasarlanmış; geliştirici ihtiyaca göre konu (topic) listeleme ekranı ekleyebilir.

## Donanım Gereksinimleri

- ESP32-S3 geliştirme kartı
- 128×128 dokunmatik LCD ekran (SPI arayüzlü)
- WiFi bağlantısı olan bir ağ

## Gereksinimler

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html) (önerilen: v5.x)
- LVGL bileşeni (`idf_component.yml` üzerinden otomatik indirilir)

## Kurulum

1. Depoyu klonla:
   ```bash
   git clone https://github.com/ugurakas/LVGL-ESP32S3.git
   cd LVGL-ESP32S3
   ```

2. Secrets dosyasını oluştur (API kimlik bilgileri için):
   ```bash
   cp secrets.h.example secrets.h
   ```
   `secrets.h` içindeki `REPLACE_ME` alanlarını kendi API bilgilerinle doldur. Bu dosya `.gitignore` içinde olduğu için commit'lenmez.

3. ESP-IDF ortamını aktive et:
   ```bash
   . $HOME/esp/esp-idf/export.sh
   ```

4. WiFi ve diğer proje ayarlarını yapılandır:
   ```bash
   idf.py menuconfig
   ```
   `Example Configuration` menüsünden WiFi SSID/parola ve maksimum yeniden deneme sayısını gir.

5. Derle ve yükle:
   ```bash
   idf.py set-target esp32s3
   idf.py build
   idf.py -p <PORT> flash monitor
   ```

## Proje Yapısı

```
.
├── main.c              # Uygulama giriş noktası, WiFi ve HTTP mantığı
├── lv_port.c / .h       # LVGL port katmanı (ekran sürücüsü entegrasyonu)
├── config.h             # Genel event/base tanımları
├── ui/                  # LVGL ile oluşturulan arayüz dosyaları
├── CMakeLists.txt       # Bileşen build tanımı
├── Kconfig.projbuild    # menuconfig üzerinden ayarlanabilir proje seçenekleri
├── secrets.h.example    # API kimlik bilgileri şablonu (kopyalayıp doldur)
└── idf_component.yml    # ESP-IDF bileşen bağımlılıkları (LVGL vb.)
```

## Güvenlik Notu

Bu depo daha önce bir API auth token'ını doğrudan kod içinde barındırıyordu. Bu artık `secrets.h` (commit edilmeyen, gitignore'lu bir dosya) üzerinden yönetiliyor. **Eğer bu repoyu fork'ladıysan veya daha önce klonladıysan, eski commit geçmişinde token hâlâ görünür olabilir** — geçmişi `git filter-repo` veya BFG Repo-Cleaner ile temizlemen ve ilgili API token'ını sunucu tarafında iptal edip yenilemen önerilir.

## Yol Haritası

- [ ] MQTT desteği (HTTP polling yerine/yanında)
- [ ] Dinamik topic/konu listeleme ekranı
- [ ] Proje klasör yapısının sadeleştirilmesi

## Katkıda Bulunma

Pull request'ler ve issue'lar memnuniyetle karşılanır.

## Lisans

Belirtilmemiş — bir lisans eklemek istersen [choosealicense.com](https://choosealicense.com/) üzerinden uygun bir lisans seçip `LICENSE` dosyası olarak ekleyebilirsin.
