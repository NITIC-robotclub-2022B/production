#include "mbed.h"
#include "PS3.h"
#include "mbed_wait_api.h"
#include "QEI.h"

#define ADDRESS_MIGI_UE 0x14
#define ADDRESS_MIGI_SITA 0x22
#define ADDRESS_HIDARI_UE 0x40
#define ADDRESS_HIDARI_SITA 0x30

#define ADDRESS_ROLLER 0x24
#define ADDRESS_RACKandPINION 0x18

#define ADDRESS_ANGLECHANGE_VERTICAL 0x00
#define ADDRESS_ANGLECHANGE_HORIZONTAL 0x60

I2C i2c(D14,D15);
PS3 ps3 (A0,A1);
DigitalOut sig(D13);
DigitalOut Air(PC_8);

void send(char add,char data);
void send_asimawari(char d_mu, char d_ms, char d_hs, char d_hu);
void get_data(void);
char move_value(double value);

int Ry;            //ジョイコン　右　y軸
int Rx;            //ジョイコン　右　x軸
int Ly;            //ジョイコン　左　y軸
int Lx;            //ジョイコン　左　x軸
bool L1;            //L1
bool R1;            //R1
bool L2;            //L2
bool R2;            //R2
bool button_ue;     // ↑
bool button_sita;   // ↓
bool button_migi;   // →
bool button_hidari; // ←

bool button_maru;    // 〇
bool button_sankaku; // △
bool button_sikaku;  // ☐
bool button_batu;    // ✕

char motordata_asimawari;
char motordata_anglechange_horizontal;

int main(){

    bool moved_asimawari;
    bool moved_anglechange_horizontal;

    while (true) {
        Air = 0;

        //緊急停止
        if(ps3.getSELECTState()){
            sig = 1;
        }

        if(ps3.getSTARTState()){
            sig = 0;
        }

        get_data();
        printf("m:%d L:%d R:%d Lx%d Ly%d\n",button_maru,L1,R1,Lx,Ly);
        
        //ジョイコン処理
        moved_asimawari = 0;
        moved_anglechange_horizontal = 0;

        if(Lx != 0 || Ly != 0){
            double value_ru,value_rs,value_lu,value_ls;   //右上,右下,左上,左下
            char data_ru,data_rs,data_lu,data_ls;

            if(Lx==-64) Lx = -63;
            if(Ly==64) Ly = 63;

            //右回転を正としたときのベクトル値計算
            value_ru = 0.7071*((double)Ly-(double)Lx)*-1;//逆転
            value_ls = 0.7071*((double)Ly-(double)Lx);   //正転
            value_rs = 0.7071*((double)Ly+(double)Lx)*-1;//逆転
            value_lu = 0.7071*((double)Ly+(double)Lx);   //正転

            data_ru = move_value(value_ru);
            data_rs = move_value(value_rs);
            data_lu = move_value(value_lu);
            data_ls = move_value(value_ls);
            
            send_asimawari(data_ru, data_rs, data_ls, data_lu);
            moved_asimawari = 1;
        }

        //機体左に旋回
        if(L1 && !moved_asimawari){
            motordata_asimawari = 0x60;
            send_asimawari(motordata_asimawari,motordata_asimawari,motordata_asimawari,motordata_asimawari);
            moved_asimawari = 1;
        }

        //機体右に旋回
        if(R1 && !moved_asimawari){
            motordata_asimawari = 0xa0;
            send_asimawari(motordata_asimawari,motordata_asimawari,motordata_asimawari,motordata_asimawari);
            moved_asimawari = 1;
        }
        
        //砲台を左に旋回
        if(L2 && !moved_anglechange_horizontal){
            motordata_anglechange_horizontal = 0x20;
            send(ADDRESS_ANGLECHANGE_HORIZONTAL, motordata_anglechange_horizontal);
            moved_anglechange_horizontal = 1;
        }
        //砲台を右に旋回
        if(R2 && !moved_anglechange_horizontal){
            motordata_anglechange_horizontal = 0xd0;
            send(ADDRESS_ANGLECHANGE_HORIZONTAL, motordata_anglechange_horizontal);
            moved_anglechange_horizontal = 1;
        }

        if(button_maru){
            send(ADDRESS_ROLLER, 0x5e);
        }

        if(button_sankaku){
            Air = 1;
        }

        //足回り静止
        if(!moved_asimawari){
            motordata_asimawari = 0x80;  
            send_asimawari(motordata_asimawari,motordata_asimawari,motordata_asimawari,motordata_asimawari);
        }
        //砲台停止
        if(!moved_anglechange_horizontal){
            motordata_anglechange_horizontal = 0x80;
            send(ADDRESS_ANGLECHANGE_HORIZONTAL, motordata_anglechange_horizontal);
        }

        
        
    }
}

//データ取得のための関数
void get_data(void){
    //ボタン取得
    button_ue = ps3.getButtonState(PS3::ue);
    button_sita= ps3.getButtonState(PS3::sita);
    button_migi = ps3.getButtonState(PS3::migi);
    button_hidari = ps3.getButtonState(PS3::hidari);

    L1 = ps3.getButtonState(PS3::L1);
    R1 = ps3.getButtonState(PS3::R1);
    L2 = ps3.getButtonState(PS3::L2);
    R2 = ps3.getButtonState(PS3::R2);

    button_maru = ps3.getButtonState(PS3::maru);

    //スティックの座標取得
    Ry = ps3.getRightJoystickYaxis();
    Rx = ps3.getRightJoystickXaxis();
    Ly = ps3.getLeftJoystickYaxis();
    Lx = ps3.getLeftJoystickXaxis();
}


void send(char address, char data){
    wait_us(15000);
    i2c.start();
    i2c.write(address);
    i2c.write(data);
    i2c.stop();
}

void send_asimawari(char d_mu, char d_ms, char d_hs, char d_hu){
    send(ADDRESS_MIGI_UE,d_mu);
    send(ADDRESS_MIGI_SITA,d_ms);
    send(ADDRESS_HIDARI_SITA,d_hs);
    send(ADDRESS_HIDARI_UE,d_hu);
}
//取得座標から回転速度を求める関数
char move_value(double value){
    double uv = value;
    int rate, move;
    char result;

    //絶対値化
    if(value<0) uv = uv*-1;

    //割合計算(小数点以下四捨五入=>整数)
    //座標の絶対値の最高値が63と64になるので小さいほうの63に合わせて割合を出す
    rate = (double)uv/89.0954*100+0.5;
    
    //64だと100%を超えるのでその調整
    if(rate>100) rate = 100;

    //割合をもとに出力するパワーを計算(整数)
    rate = (double)rate/100.0*123.0;
    
    //正転と逆転の処理
    //引数がマイナスの時は逆転、プラスの時は正転、0の時には静止
    if(!value) move = 128;
    else if(value<0) move = 124-rate;
    else move = rate + 132;

    //型変更
    result = (char)move;
    return result;
}