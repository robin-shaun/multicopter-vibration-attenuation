#include <avr/pgmspace.h>
#include <MPUDriver.h>
#include <TimerOne.h>
#include <SPI.h>
#include <SD.h>

#define DEFAULT_MPU_HZ 1000
#define SAMPLEINT 1000
#define MPUNUM 1
#define SpeedTestInterval 500  //ms
#define SampleInterval 1000
#define PulseRate 7
#define DATAFILE "data078.txt"
#define SPEEDFILE "speedlog6.txt"

const uint8_t ncs_l[MPUNUM]={48};
const int sdSelect=49;
const int SampleSwitchPin =19;

int logSpeedFlag=0;
long samplePulseTime=0;
MPUDriver mpu;


int mpu_init_state =1;
int mpu_dmp_state =1;
int dmp_load_state =1;

unsigned long sensor_timestamp=0;
long lastSampleTime=0;
short gyro[3],accel[3];
long quat[4];
unsigned char more,sensors;
long speed_count[4]={0};

uint8_t read_r;
/*** Function Declear ***/
void calculate_speed();
void sampleSwitchInt();
void speedPulse0();
void speedPulse1();
void speedPulse2();
void speedPulse3();
void SampleF();
void(* resetFunc)(void)=0;
/*******  SETUP *******/
void setup() {
  Serial.begin(115200);
  Timer1.initialize(SampleInterval);
  Timer1.attachInterrupt(SampleF);
  attachInterrupt(0,speedPulse0,RISING);
  attachInterrupt(4,sampleSwitchInt,CHANGE);
  while (!Serial);
  //Initialize SD Card
 if(!SD.begin(sdSelect)){
    Serial.println("Card failed, or not present");
    delay(500);
    resetFunc();
  }
  Serial.println("Card Initialized");
  delay(500);
  //Initialize MPU Select pin
    mpu_init_state=mpu.mpu_init(0);//Without Interrupt
    if(mpu_init_state){
      Serial.print(HW_addr);
      Serial.println("  MPU Init Failed");
      delay(100);
      resetFunc();      
    }
    mpu.mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    mpu.mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    mpu.mpu_set_sample_rate(DEFAULT_MPU_HZ);
    //offset
    long gyr_bias[3] = { 0, 0, };
    mpu.mpu_set_gyro_bias_reg(gyr_bias);  
    long acc_bias[3] = {210,20, -300};
    mpu.mpu_set_accel_bias_6050_reg(acc_bias);
    {
      Serial.print(HW_addr);
      Serial.println("  MPU Init Successfully");
    }
    delay(500);
 pinMode(22,OUTPUT);
 pinMode(23,OUTPUT);
 digitalWrite(22,HIGH);
 digitalWrite(23,LOW);
}
long count=0;
long lasttime=micros();
String dataString="";
String speedString="";
File dataFile;
//File dataFile=SD.open(DATAFILE,FILE_WRITE);
File speedLog;
/*****  LOOP *****/
void loop() {
  //Serial.println(samplePulseTime);
  if(samplePulseTime>1500){
      Serial.println("LOCKED");
      while(samplePulseTime>1500){
        delay(500);
        if(!SD.begin(sdSelect)){
          Serial.println("Card failed, or not present");
          delay(500);
          resetFunc();
        }
        Serial.println("LOCKED");
      }
     if(!SD.begin(sdSelect)){
        Serial.println("Card failed, or not present");
        delay(500);
        resetFunc();
      }
      Serial.println("UNLOCK");
    }
  if(count==200){
    count=0;
    //Serial.println("-------------------");
    Serial.println(micros()-lasttime);
    dataFile.close();
    dataFile=SD.open(DATAFILE,FILE_WRITE);
    lasttime=micros();
  }
  /*Serial.print("MPU:");
  Serial.print(cur_mpu);
  Serial.print("\t");*/
  
  //memset(gyro, 0, 6);
  memset(accel, 0, 6);
  //memset(&sensors, 0, 1);
  //memset(&read_r, 0, 1);
  read_r = mpu.mpu_get_accel_reg(accel,&sensor_timestamp);
   if (!read_r)
  {
//      gyro_f[0] = (float)gyro[0]  / 32768 * 200;
//      gyro_f[1] = (float)gyro[1]  / 32768 * 200;
//      gyro_f[2] = (float)gyro[2]  / 32768 * 200;
     /* Serial.print("\t");
      Serial.print(gyro[0]);
      Serial.print("\t");
      Serial.print(gyro[1]);
      Serial.print("\t");
      Serial.print(gyro[2]);
      //accel_f[0] = (float)accel[0] / 32768 * 2;
      //accel_f[1] = (float)accel[1] / 32768 * 2;
      //accel_f[2] = (float)accel[2] / 32768 * 2;
      Serial.print("\t");
      *///Serial.print("# ");
    /*  Serial.print(accel[0]);
      Serial.print("\t");
      Serial.print(accel[1]);
      Serial.print("\t");
      Serial.println(accel[2]);*/
  }
  else{
    Serial.print("ERROE:");
    Serial.println(read_r);
  }
  dataString+=String(accel[0])+","+String(accel[1])+","+String(accel[2])+",";
  // File dataFile=SD.open("datalog4.txt",FILE_WRITE);
  // if(dataFile){
      dataFile.println("");
      dataFile.print(dataString);
      if(logSpeedFlag){
        logSpeedFlag=0;
        dataFile.print(speedString);
        speedString="";
      }
     // dataFile.close();
      //Serial.print(cur_mpu);
      //Serial.println(dataString);
    dataString="";
  //}
  count++;
}//loop
void SampleF(){
   //memset(gyro, 0, 6);
  memset(accel, 0, 6);
  //memset(&sensors, 0, 1);
  //memset(&read_r, 0, 1);
  read_r = mpu.mpu_get_accel_reg(accel,&sensor_timestamp);
   if (!read_r)
  {
  }
  else{
    Serial.print("ERROE:");
    Serial.println(read_r);
  }
  dataString+=String(accel[0])+","+String(accel[1])+","+String(accel[2])+",";
  dataFile.println("");
  dataFile.print(dataString);
  if(logSpeedFlag){
    logSpeedFlag=0;
    dataFile.print(speedString);
    speedString="";
  }
  dataString="";
  count++;
}

void calculate_speed(){
  float speed_cal[4]={0};
  for(int i=0;i<4;i++){
    speed_cal[i]=(float)speed_count[i]*1000/SpeedTestInterval/PulseRate;// r/s
    speed_count[i]=0;
  }
  speedString=String(speed_cal[0])+","+String(speed_cal[1])+","+String(speed_cal[2])+","+String(speed_cal[3])+",";
  logSpeedFlag=1;
  
}
void sampleSwitchInt(){
  static long lastIntTime=0;
  if(digitalRead(SampleSwitchPin)==LOW){
    samplePulseTime=micros()-lastIntTime;
  }
  else{
    lastIntTime=micros();
  }
}

void speedPulse0(){
  speed_count[0]+=1;
}
void speedPulse1(){
  speed_count[1]+=1;
}
void speedPulse2(){
  speed_count[2]+=1;
}
void speedPulse3(){
  speed_count[3]+=1;
}

