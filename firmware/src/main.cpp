#include <Arduino.h>
#include "combo/BleCombo.h"
#include <Adafruit_NeoPixel.h>

#include "I2Cdev.h"

#include "MPU6050_6Axis_MotionApps20.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

#define OUTPUT_READABLE_YAWPITCHROLL

#define INTERRUPT_PIN 34 // use pin 2 on Arduino Uno & most boards

#define WS2812_PIN 19 // RGB选择引脚
#define WS2812_NUM 3  // 定义led个数，决定数组长度

MPU6050 mpu;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(WS2812_NUM, WS2812_PIN, NEO_GRB + NEO_KHZ800);
BleComboKeyboard KeyBoard = BleComboKeyboard("cainiao_gun", "Espressif", 100);
BleComboMouse Mouse(&KeyBoard);

// RGB_api
void colorWipe(uint32_t color, int wait);
void theaterChase(uint32_t color, int wait);
void rainbow(int wait);
void theaterChaseRainbow(int wait);
unsigned long _millis = 0;
int RGB_time = 0;
// RGB随机数
int R;
int G;
int B;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;        // [w, x, y, z]         quaternion container
VectorInt16 aa;      // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;  // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld; // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity; // [x, y, z]            gravity vector
float euler[3];      // [psi, theta, phi]    Euler angle container
float ypr[3];        // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

int16_t ax, ay, az, gx, gy, gz;

// packet structure for InvenSense teapot demo
uint8_t teapotPacket[14] = {'$', 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x00, '\r', '\n'};

volatile bool mpuInterrupt = false; // indicates whether MPU interrupt pin has gone high
void dmpDataReady()
{
    mpuInterrupt = true;
}

// IO定义
const int x = 0;
const int y = 4;
const int Key_sw = 23;
const int Key_1 = 12;
const int Key_2 = 32;
const int Key_3 = 33;
const int Key_4 = 35;
const int Key_5 = 17;
const int Key_6 = 16;
const int Key_7 = 18;
const int Key_8 = 5;

// 键码定义
/*
16//shift
119//w
97//a
100//d
115//s
87//shift+w
83//shift+s
68//shift+d
65//shift+a
32//space
114//r
102//f
*/

int num1 = 0; // 电磁后坐计数
int num2 = 0; // RGB计数

uint8_t moveX = 0, moveY = 0, IO5 = 0, IO18 = 0, IO16 = 0, IO17 = 0, pat = 0;
int16_t mouseX = 0, mouseY = 0;
uint8_t cnt1 = 1, cnt2 = 1, num_sw = 1, num_key1 = 1, num_key2 = 1, num_key3 = 1, num_en1 = 1, num_en2 = 1;

// 触摸消抖
static bool T32()
{
    static int press_cnt1 = 0;
    bool ret_cnt1 = false;
    if (touchRead(32) < 10)
    {
        press_cnt1++;
        if (press_cnt1 > 6)
        {
            if (touchRead(32) < 10)
            {
                ret_cnt1 = true;
                press_cnt1 = -8;
            }
        }
    }
    else
    {
        press_cnt1 = 0;
    }
    return ret_cnt1;
}

static bool T33()
{
    static int press_cnt2 = 0;
    bool ret_cnt2 = false;
    if (touchRead(33) < 15)
    {
        press_cnt2++;
        if (press_cnt2 > 4)
        {
            if (touchRead(33) < 15)
            {
                ret_cnt2 = true;
                press_cnt2 = -8;
            }
        }
    }
    else
    {
        press_cnt2 = 0;
    }
    return ret_cnt2;
}

// mpu6050_DMP滤波
void mpu_DMP()
{
    // if programming failed, don't try to do anything
    if (!dmpReady)
        return;
    // read a packet from FIFO
    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer))
    { // Get the Latest packet

#ifdef OUTPUT_READABLE_YAWPITCHROLL
      // display Euler angles in degrees
        // mpu.dmpGetQuaternion(&q, fifoBuffer);
        // mpu.dmpGetGravity(&gravity, &q);
        // mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        // Serial.print("ypr\t");
        // Serial.print(ypr[0] * 180/M_PI);
        // Serial.print("\t");
        // Serial.print(ypr[1] * 180/M_PI);
        // Serial.print("\t");
        // Serial.println(ypr[2] * 180/M_PI);

        // Serial.print("ypr\t");
        // Serial.print(gy/150);
        // Serial.print("\t");
        // Serial.print(ypr[2] * 180/M_PI);
        // Serial.print("\t");
        // Serial.println(gz/150);
        delay(9);

        // mouseX=-gz/150;
        // mouseY=-gy/150;

#endif
    }
}

// 模式切换
void pattern_ctrl()
{
    if (digitalRead(Key_8))
    {
        pat = 1;
    }
    else
    {
        pat = 0;
    }
}

// 模式功能
void pattern()
{
    if (pat == 0)
    {
        mouseX = ypr[0] * 180 / M_PI / 15;
        mouseY = ypr[1] * 180 / M_PI / 15;
    }
    else
    {
        mouseX = -gz / 250;
        mouseY = -gy / 250;
    }
}

// 电磁后坐
void houzuo()
{
    if (num1 == 80)
    {
        digitalWrite(25, HIGH);
        // Serial.println(111);
    }
    if (num1 == 120)
    {
        digitalWrite(25, LOW);
        num1 = 0;
    }
    num1++;
}

// RGB
void RGB()
{
    // RGB随机数
    R = random(255);
    G = random(255);
    B = random(255);
    if (num2 == 80)
    {
        strip.setPixelColor(0, R, G, B);
        strip.setPixelColor(1, B, R, G);
        strip.setPixelColor(2, G, B, R);
        strip.show();
    }
    if (num2 == 120)
    {
        strip.clear(); // 关闭所有灯
        strip.show();
        num2 = 0;
    }
    num2++;
}

// 键码定义
/*
16//shift
119//w
97//a
100//d
115//s
87//shift+w
83//shift+s
68//shift+d
65//shift+a
32//space
114//r
102//f
*/

// 摇杆移动
void encoder()
{
    int x_value = analogRead(x), y_value = analogRead(y);

    if (x_value > 3500)
    {
        if (num_en1)
        {
            KeyBoard.press(16);
            KeyBoard.press(119);
            moveX = 1;
            num_en1 = 0;
        }
    }
    else if (x_value < 500)
    {
        if (num_en1)
        {
            KeyBoard.press(16);
            KeyBoard.press(115);
            moveX = -1;
            num_en1 = 0;
        }
    }
    else
    {
        if (num_en1 == 0)
        {
            KeyBoard.release(87);
            KeyBoard.release(83);
            KeyBoard.press(16);
            num_en1 = 1;
        }
    }
    if (y_value > 3900)
    {
        if (num_en2)
        {
            KeyBoard.press(16);
            KeyBoard.press(100);
            moveY = 1;
            num_en2 = 0;
        }
    }
    else if (y_value < 500)
    {
        if (num_en2)
        {
            KeyBoard.press(16);
            KeyBoard.press(97);
            moveY = -1;
            num_en2 = 0;
        }
    }
    else
    {
        if (num_en2 == 0)
        {
            KeyBoard.release(68);
            KeyBoard.release(65);
            KeyBoard.press(16);
            num_en2 = 1;
        }
    }
}

// 鼠标移动
void mouse_Move(int16_t _mouseX, int16_t _mouseY)
{
    //   if(Keyboard.isConnected()) {
    // if(mouseX!=0|mouseY!=0){
    delay(3);
    for (uint8_t _i = 0; _i < 4; _i++)
    {
        Mouse.move(_mouseX, -_mouseY);
    }

    // }
    //   }
}

// 鼠标中键
void key_sw()
{
    if (digitalRead(Key_sw) == 0)
    {
        if (num_sw)
        {
            Mouse.press(MOUSE_MIDDLE);
            num_sw = 0;
        }
    }
    else
    {
        if (num_sw == 0)
        {
            Mouse.release(MOUSE_MIDDLE);
            num_sw = 1;
        }
    }
}

// 鼠标左键
void mouse_Left()
{
    if (digitalRead(Key_5))
    {
        if (cnt2)
        {
            Mouse.press(MOUSE_LEFT);
            cnt2 = 0;
        }
        houzuo();
        RGB();
    }
    else
    {
        if (cnt2 == 0)
        {
            Mouse.release(MOUSE_LEFT);
            cnt2 = 1;
        }
    }
}

// 鼠标右键
void mouse_Right()
{
    if (digitalRead(Key_6))
    {
        if (cnt1)
        {
            Mouse.press(MOUSE_RIGHT);
            cnt1 = 0;
        }
    }
    else
    {
        if (cnt1 == 0)
        {
            Mouse.release(MOUSE_RIGHT);
            cnt1 = 1;
        }
    }
}

void key_1()
{
    if (digitalRead(Key_1))
    {
        if (num_key1)
        {
            Mouse.press(32);
            num_key1 = 0;
        }
    }
    else
    {
        if (num_key1 == 0)
        {
            Mouse.release(32);
            num_key1 = 1;
        }
    }
}

void key_2()
{
    if (digitalRead(Key_2))
    {
        if (num_key2)
        {
            Mouse.press(114);
            num_key2 = 0;
        }
    }
    else
    {
        if (num_key2 == 0)
        {
            Mouse.release(114);
            num_key2 = 1;
        }
    }
}

void key_3()
{
    if (digitalRead(Key_3))
    {
        if (num_key3)
        {
            Mouse.press(102);
            num_key3 = 0;
        }
    }
    else
    {
        if (num_key3 == 0)
        {
            Mouse.release(102);
            num_key3 = 1;
        }
    }
}

// 参数打印
void _print()
{
    Serial.println(
        // "moveX:"+String(moveX)+
        // "  moveY:"+String(moveY)+
        "  mouseX:" + String(mouseX) +
        "  mouseY:" + String(mouseY)
        // "  1:"+String(digitalRead(12))+
        // "  2:"+String(digitalRead(32))+
        // "  3:"+String(digitalRead(33))+
        // "  4:"+String(digitalRead(35))+
        // "  5:"+String(digitalRead(17))+
        // "  6:"+String(digitalRead(16))+
        // "  7:"+String(digitalRead(18))+
        // "  8:"+String(digitalRead(5))+
        // "  sw:"+String(digitalRead(Key_sw))
        // "  pat:"+String(pat)
    );
}

// RGB功能实现
//  Some functions of our own for creating animated effects -----------------
//  Fill strip pixels one after another with a color. Strip is NOT cleared
//  first; anything there will be covered pixel by pixel. Pass in color
//  (as a single 'packed' 32-bit value, which you can get by calling
//  strip.Color(red, green, blue) as shown in the loop() function above),
//  and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait)
{
    for (int i = 0; i < strip.numPixels(); i++)
    {                                  // For each pixel in strip...
        strip.setPixelColor(i, color); //  Set pixel's color (in RAM)
        strip.show();                  //  Update strip to match
        delay(wait);                   //  Pause for a moment
    }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait)
{
    for (int a = 0; a < 10; a++)
    { // Repeat 10 times...
        for (int b = 0; b < 3; b++)
        {                  //  'b' counts from 0 to 2...
            strip.clear(); //   Set all pixels in RAM to 0 (off)
            // 'c' counts up from 'b' to end of strip in steps of 3...
            for (int c = b; c < strip.numPixels(); c += 3)
            {
                strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
            }
            strip.show(); // Update strip with new contents
            delay(wait);  // Pause for a moment
        }
    }
}

void rainbow(int wait)
{
    for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256)
    {

        strip.rainbow(firstPixelHue);
        // Above line is equivalent to:
        // strip.rainbow(firstPixelHue, 1, 255, 255, true);
        strip.show(); // Update strip with new contents
        delay(wait);  // Pause for a moment
    }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait)
{
    if (millis() > _millis)
    {
        int firstPixelHue = 0; // First pixel starts at red (hue 0)
        for (int a = 0; a < 30; a++)
        { // Repeat 30 times...
            for (int b = 0; b < 3; b++)
            {                  //  'b' counts from 0 to 2...
                strip.clear(); //   Set all pixels in RAM to 0 (off)
                // 'c' counts up from 'b' to end of strip in increments of 3...
                for (int c = b; c < strip.numPixels(); c += 3)
                {
                    // hue of pixel 'c' is offset by an amount to make one full
                    // revolution of the color wheel (range 65536) along the length
                    // of the strip (strip.numPixels() steps):
                    int hue = firstPixelHue + c * 65536L / strip.numPixels();
                    uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
                    strip.setPixelColor(c, color);                       // Set pixel 'c' to value 'color'
                }
                strip.show(); // Update strip with new contents
                // delay(wait); // Pause for a moment
                firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
                _millis = millis() + 1000;
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting work!");
    KeyBoard.begin();
    Mouse.begin();
    strip.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)把对应pin设置为输出
    strip.show();             // Turn OFF all pixels ASAP
    strip.setBrightness(100); // Set BRIGHTNESS to about 1/5 (max = 255)，设置亮度
    pinMode(Key_sw, INPUT_PULLUP);
    pinMode(12, INPUT_PULLDOWN);
    pinMode(32, INPUT_PULLDOWN);
    pinMode(33, INPUT_PULLDOWN);
    pinMode(35, INPUT_PULLDOWN);
    pinMode(17, INPUT_PULLDOWN);
    pinMode(16, INPUT_PULLDOWN);
    pinMode(18, INPUT_PULLDOWN);
    pinMode(5, INPUT_PULLDOWN);

    pinMode(25, OUTPUT); // 电磁后坐

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    Wire.begin();
    Wire.setClock(400000); // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
#endif

    while (!Serial)
        ; // wait for Leonardo enumeration, others continue immediately

    // initialize device
    Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();
    pinMode(INTERRUPT_PIN, INPUT);

    // verify connection
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

    // // wait for ready
    // Serial.println(F("\nSend any character to begin DMP programming and demo: "));
    // while (Serial.available() && Serial.read()); // empty buffer
    // while (!Serial.available());                 // wait for data
    // while (Serial.available() && Serial.read()); // empty buffer again

    // load and configure the DMP
    Serial.println(F("Initializing DMP..."));
    devStatus = mpu.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

    // make sure it worked (returns 0 if so)
    if (devStatus == 0)
    {
        // Calibration Time: generate offsets and calibrate our MPU6050
        mpu.CalibrateAccel(6);
        mpu.CalibrateGyro(6);
        mpu.PrintActiveOffsets();
        // turn on the DMP, now that it's ready
        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        // enable Arduino interrupt detection
        Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
        Serial.print(digitalPinToInterrupt(INTERRUPT_PIN));
        Serial.println(F(")..."));
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();

        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        Serial.println(F("DMP ready! Waiting for first interrupt..."));
        dmpReady = true;

        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
    }
    else
    {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
    }
}

void loop()
{
    if (KeyBoard.isConnected())
    {
        // pattern_ctrl();
        pat = 1;

        mpu_DMP();
        pattern();
        mouse_Move(mouseX, mouseY);

        // 原始程序..............................................................................

        // mouse_Right();
        // mouse_Left();
        // encoder();
        // key_sw();
        // key_1();
        // key_2();
        // key_3();
        _print();
        // moveX=0,moveY=0,mouseX=0,mouseY=0,IO5=0,IO18=0,IO16=0,IO17=0;
        // delay(1);
        //..............................................................................

        // RGB随机数
        //  int R=random(255);
        //  int G=random(255);
        //  int B=random(255);

        // if(RGB_time==1){colorWipe(strip.Color(255,   0,   0), 0);}//Red
        // if(RGB_time==100){colorWipe(strip.Color(  0, 255,   0), 0);}// Green
        // if(RGB_time==200){colorWipe(strip.Color(  0,   0, 255), 0);}// Blue
        // if(RGB_time==300){RGB_time=0;}
        // RGB_time++;

        // 摇杆：已验证
        //   int x_value = analogRead(x);
        //   int y_value = analogRead(y);
        //   Serial.print("x: ");
        //   Serial.print(x_value);
        //   Serial.print("\t");
        //   Serial.print("y: ");
        //   Serial.print(y_value);
        //   Serial.print("\t");
        //   Serial.print("sw: ");
        //   Serial.print(digitalRead(sw));
        //   Serial.println("\t");
        //   delay(200);
        //..................

        // if(Keyboard.isConnected()) {
        //   Serial.println("Sending 'Hello world'");
        //   // Keyboard.println("Hello World");

        //   delay(1000);
        //   Serial.println("Sending Enter key...");

        //   if(digitalRead(34) == 1){Serial.println(1);}
        //   if(digitalRead(34) == 0){Serial.println(0);}
        //.................
        // Keyboard.write(KEY_RETURN);
        // Keyboard.press(99);
        // delay(50);
        // Keyboard.releaseAll();
        //......................

        // delay(1000);

        // Serial.println("Sending Play/Pause media key...");
        // Keyboard.write(KEY_MEDIA_PLAY_PAUSE);

        // delay(1000);

        // Serial.println("Sending Ctrl+Alt+Delete...");
        // Keyboard.press(KEY_LEFT_CTRL);
        // Keyboard.press(KEY_LEFT_ALT);
        // Keyboard.press(KEY_DELETE);
        // delay(100);
        // Keyboard.releaseAll();

        //.........................
        // unsigned long startTime;

        // Serial.println("Move mouse pointer up");
        // startTime = millis();
        // while(millis()<startTime+1000) {
        //   Mouse.move(0,-1);
        //   delay(5);
        // }
        //............................

        // Serial.println("Move mouse pointer left");
        // startTime = millis();
        // while(millis()<startTime+1000) {
        //   Mouse.move(-1,0);
        //   delay(5);
        // }

        // Serial.println("Move mouse pointer down");
        // startTime = millis();
        // while(millis()<startTime+1000) {
        //   Mouse.move(0,1);
        //   delay(5);
        // }

        // Serial.println("Move mouse pointer right");
        // startTime = millis();
        // while(millis()<startTime+1000) {
        //   Mouse.move(1,0);
        //   delay(5);
        // }

        // Serial.println("Scroll Down");
        // Mouse.move(0,0,-1);

        // Serial.println("Left click");
        // Mouse.click(MOUSE_LEFT);
        // delay(500);

        // Serial.println("Right click");
        // Mouse.click(MOUSE_RIGHT);
        // delay(500);

        // Serial.println("Scroll wheel click");
        // Mouse.click(MOUSE_MIDDLE);
        // delay(500);

        // Serial.println("Back button click");
        // Mouse.click(MOUSE_BACK);
        // delay(500);

        // Serial.println("Forward button click");
        // Mouse.click(MOUSE_FORWARD);
        // delay(500);

        // Serial.println("Click left+right mouse button at the same time");
        // Mouse.click(MOUSE_LEFT | MOUSE_RIGHT);
        // delay(500);

        // Serial.println("Click left+right mouse button and scroll wheel at the same time");
        // Mouse.click(MOUSE_LEFT | MOUSE_RIGHT | MOUSE_MIDDLE);
        // delay(500);

        // }

        // Serial.println("Waiting 2 seconds...");
        // delay(2000);
    }
}
