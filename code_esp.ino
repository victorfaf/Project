#include <FirebaseESP8266.h>
#include <ESP8266WiFi.h>
#include <Adafruit_Fingerprint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
// Thông tin Wi-Fi
#define WIFI_SSID "iPhone0"
#define WIFI_PASSWORD "88888889"
// Cài đặt NTP
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 25200); 
// Thông tin Firebase
#define FIREBASE_HOST "diemdanhvantay-default-rtdb.firebaseio.com"
#define FIREBASE_SECRET "0wV5EkXuv5m2SXGZWD03prsxzQ4C0nNfWMd5qv5Y"  // Thay thế bằng Database Secret của bạn

FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth; // Tạo đối tượng FirebaseAuth rỗng

// Cấu hình màn hình OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Cấu hình cảm biến vân tay
#define fingerprint_RX D5
#define fingerprint_TX D6
SoftwareSerial mySerial(fingerprint_RX, fingerprint_TX);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

bool isIdentifying = true;  // Đặt mặc định là chế độ nhận diện liên tục

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Kết nối WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected!");
    // Bắt đầu NTP Client
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_SECRET; // Sử dụng Database Secret
  Firebase.begin(&config, &auth); // Truyền cả hai tham số

  // Khởi tạo cảm biến vân tay
  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Cảm biến vân tay sẵn sàng!");
  } else {
    Serial.println("Không tìm thấy cảm biến vân tay :(");
    while (1);
  }
   // Khởi tạo timeClient
  timeClient.begin();//

  // Đợi cho đến khi thời gian được đồng bộ hóa
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  // Lấy thời gian từ NTP server
  unsigned long epochTime = timeClient.getEpochTime();
  setTime(epochTime); // Cài đặt thời gian hệ thống từ epoch

  Serial.println("Thời gian đã được đồng bộ.");//
  // Khởi tạo màn hình OLED
  if (Firebase.ready()) {
  Serial.println("Đã kết nối Firebase!");
} else {
  Serial.println("Chưa kết nối Firebase!");
}
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Lỗi màn hình OLED!");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("He thong van tay san sang!");
  display.display();
}

void loop() {
  // Cập nhật thời gian
  timeClient.update();
   // Lấy thời gian
  // Chuyển thời gian Epoch sang ngày tháng giờ phút
  int year1 = year();
  int month1 = month();
  int day1 = day();
  int hour1 = hour();
  int minute1 = minute();
  // In ra kết quả theo định dạng yyyy-MM-dd HH:mm
  String formattedTime = String(year1) + "-" + String(month1) + "-" + String(day1) + " " + String(hour1) + ":" + String(minute1);
  // Kiểm tra nếu có dữ liệu từ Serial Monitor
if (Firebase.getString(firebaseData, "/data/themvantay")) {
    String themvantay = firebaseData.stringData();
    // Nếu có dữ liệu và nó không rỗng, chuyển đổi thành int
    if (themvantay != "0") {
      int id = themvantay.toInt();  // Chuyển giá trị thành int
      isIdentifying = false;  // Dừng chế độ nhận diện liên tục
      enrollFinger(id);  // Gọi hàm thêm vân tay với ID chỉ định
      String path1 = ("/data/themvantay");
      Firebase.setString(firebaseData, path1, "0");
      
    } 
    else if (Firebase.getString(firebaseData, "/data/xoavantay")) {
    String xoavantay = firebaseData.stringData();
    // Nếu có dữ liệu và nó không rỗng, chuyển đổi thành int
    if (xoavantay != "0") {
      int id = xoavantay.toInt();  // Chuyển giá trị thành int
      isIdentifying = false;  // Dừng chế độ nhận diện liên tục
      deleteFinger(id);  // Gọi hàm xóa vân tay với ID chỉ định
      String path2 = ("/data/xoavantay");
      Firebase.setString(firebaseData, path2, "0");
       
    } 
    }
  }

  // Nếu ở chế độ nhận diện liên tục
  if (isIdentifying) {
    uint8_t id = getFingerprintID();
    if (id != FINGERPRINT_NOTFOUND) {
      Serial.print("Tim thay van tay ID #");
      Serial.println(id);
      // Truy cập Firebase để lấy thông tin sinh viên
      String tenSV,msv;
      String studentPath = "/students/" + String(id);  // Dựa trên ID vân tay
      Firebase.get(firebaseData, studentPath);// Kiểm tra dữ liệu trả về là JSON
      if (Firebase.getString(firebaseData, studentPath + "/TENSV")) {
        tenSV = firebaseData.stringData();  // Lấy tên sinh viên từ Firebase
        Serial.println("Ten sinh vien: " + tenSV);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Ten: ");
        display.println(tenSV);
      } else {
        Serial.println("Loi khi lay TenSV");
      }

      if (Firebase.getString(firebaseData, studentPath + "/MSV")) {
        msv = firebaseData.stringData();  // Lấy mã số sinh viên từ Firebase
        Serial.println("MSSV: " + msv);
        display.print("MSV: ");
        display.println(msv);
      } else {
        Serial.println("Loi khi lay MSV");
      }
       String ttPath = studentPath + "/TT";
       String tgddPath = studentPath + "/TGDD";
       Firebase.setString(firebaseData, ttPath, "Có mặt");
       Firebase.setString(firebaseData, tgddPath, formattedTime);
        display.display();
    }
    delay(1000);  // Tạm dừng một thời gian trước khi tiếp tục nhận diện
  }
}

// Hàm nhận diện vân tay
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return FINGERPRINT_NOTFOUND;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return FINGERPRINT_NOTFOUND;

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    // Nếu tìm thấy vân tay
    Serial.print("Tim thay van tay ID #");
    Serial.println(finger.fingerID);
    return finger.fingerID;  // Trả về ID vân tay nếu tìm thấy
  } else {
    // Nếu không tìm thấy vân tay
    Serial.println("Khong tim thay van tay");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Van tay chua duoc dang ky");
    display.display();
    return FINGERPRINT_NOTFOUND;  // Không tìm thấy vân tay phù hợp
  }
}

// Hàm thêm vân tay mới với ID chỉ định
void enrollFinger(int id) {
  Serial.print("Them van tay ID #");
  Serial.println(id);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Them van tay ID #");
  display.println(id);
  display.display();
  String path = "/students/" + String(id) + "/Vantay";
  Firebase.get(firebaseData, path);
  if (firebaseData.dataType() == "string" && firebaseData.stringData() == "Đã có") {
    // Nếu đã có vân tay
    Serial.println("ID đã có vân tay");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ID da co van tay");
    display.display();
    isIdentifying = true;
    return;
  }
  // Tiến hành thêm vân tay
  int p = -1;
  Serial.println("Dat ngon tay len cam bien...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      Serial.println("Khong phat hien ngon tay");
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Khong phat hien ngon tay");
      display.display();
    } else if (p == FINGERPRINT_OK) {
      Serial.println("Da chup hinh anh vân tay lần 1");
    }
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Loi khi chuyen doi lan 1");
    isIdentifying = true;
    return;
  }
  delay(1000);
  p = -1;
  Serial.println("Dat lai ngon tay len cam bien...");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      Serial.println("Khong phat hien ngon tay");
    } else if (p == FINGERPRINT_OK) {
      Serial.println("Da chup hinh anh van tay lần 2");
    }
  }
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Loi khi chuyen doi lan 2");
    isIdentifying = true;
    return;
  }
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Loi khi tao mo hinh van tay");
    isIdentifying = true;
    return;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Da luu van tay thanh cong!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Them van tay thanh cong!");
    display.display();
    String path = "/students/" + String(id) + "/Vantay";
    Firebase.setString(firebaseData, path, "Đã có");
  } else {
    Serial.println("Loi khi luu van tay");
  }
  isIdentifying = true;
}

// Hàm xóa vân tay theo ID
void deleteFinger(int id) {
  Serial.print("Dang xoa van tay ID #");
  Serial.println(id);
  int p = finger.deleteModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Da xoa van tay thanh cong");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Xoa van tay ID #");
    display.println(id);
    display.display();
  String path = "/students/"+ String(id); // Đường dẫn cần kiểm tra
   if (Firebase.get(firebaseData, path)) { 
    // Cập nhật Firebase: Đánh dấu là "Chưa có" vân tay
    String path = "/students/" + String(id) + "/Vantay";
    Firebase.setString(firebaseData, path, "Chưa có");}
  } else {
    Serial.println("Loi khi xoa van tay");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Loi khi xoa van tay");
    display.display();
  }
  // Sau khi xóa xong vân tay, quay lại chế độ nhận diện liên tục
  isIdentifying = true;
}
