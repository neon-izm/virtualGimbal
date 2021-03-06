#ifndef VSP_H
#define VSP_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <boost/math/quaternion.hpp>
#include "settings.h"

#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
using namespace std;
using namespace boost::math;


// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
//extern GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include "seekablevideocapture.h"



class vsp
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    float fs = 0.0;
    float fc = 0.0;
    /**
     * @brief コンストラクタ
     * @param [in]	Qa	ジャイロの角速度から計算したカメラの方向を表す回転クウォータニオン時系列データ、参照渡し
     * @param [in]	Qf	LPFを掛けて平滑化した回転クウォータニオンの時系列データ、参照渡し
     * @param [in]	m	画面の縦の分割数[ ]
     * @param [in]	n	画面の横の分割数[ ]
     * @param [in]	IK	"逆"歪係数(k1,k2,p1,p2)
     * @param [in]	matIntrinsic	カメラ行列(fx,fy,cx,cy) [pixel]
     * @param [in]	imageSize	フレーム画像のサイズ[pixel]
     * @param [in]  adjustmentQuaternion 画面方向を微調整するクォータニオン[rad]
     * @param [in]	zoom	倍率[]。拡大縮小しないなら1を指定すること。省略可
     * @param [out] error はみ出したノルムの長さ
     **/
    vsp(/*vector<Eigen::Quaternion<T>> &angle_quaternion,*/
                           int32_t division_x,
                           int32_t division_y,
                           double TRollingShutter,
                           Eigen::MatrixXd IK,
                           Eigen::MatrixXd matIntrinsic,
                           int32_t image_width,
                           int32_t image_height,
                           double zoom,
                           std::vector<Eigen::Vector3d,Eigen::aligned_allocator<Eigen::Vector3d>> &angular_velocity,
                           double T_video,
                           double T_angular_velocity,
                           double frame_offset,
                           int32_t video_frames,
                           int32_t filter_tap_length = 399);/*{
        is_filtered=false;
        this->division_x = division_x;
        this->division_y = division_y;
        this->TRollingShutter = TRollingShutter;
        this->IK = IK;
        this->matIntrinsic = matIntrinsic;
        this->image_width = image_width;
        this->image_height = image_height;
        this->zoom = zoom;

        this->angular_velocity = angular_velocity;
        this->T_video = T_video;
        this->T_angular_velocity = T_angular_velocity;
        this->frame_offset = frame_offset;

        //クォータニオンをクラスの内部で計算する
//        vector<Eigen::Quaternion<double>> angleQuaternion_vsp2;
        raw_quaternion_vec.clear();
        raw_quaternion_vec.push_back(Eigen::Quaterniond(1,0,0,0));
        for(int frame= -floor(filter_tap_length/2)-1 ,e=video_frames+floor(filter_tap_length/2)+1;frame<e;++frame){//球面線形補間を考慮し前後各1フレーム追加
            auto v_sync = angularVelocitySync(frame);
//            Eigen::Vector3d ve_sync(v_sync[0],v_sync[1],v_sync[2]);
            cout << "frame:" << frame << " v_sync:" << v_sync.transpose() << endl;
            raw_quaternion_vec.push_back((raw_quaternion_vec.back()*vsp::RotationQuaternion(v_sync*this->T_video)).normalized());
//            raw_quaternion_vec.back() = raw_quaternion_vec.back().normalized();
        }


        raw_angle.resize(raw_quaternion_vec.size(),3);

        Eigen::Vector3d el = Quaternion2Vector(raw_quaternion_vec[0].conjugate());
        for(int i=0,e=raw_quaternion_vec.size();i<e;++i){
            el = Quaternion2Vector(raw_quaternion_vec[i].conjugate(),el);//require Quaternion2Matrix<3,1>()
            raw_angle(i,0) = el[0];
            raw_angle(i,1) = el[1];
            raw_angle(i,2) = el[2];
        }

        raw_quaternion.resize(raw_quaternion_vec.size(),4);
        for(int32_t i=0,e=raw_quaternion_vec.size();i<e;++i){
            raw_quaternion.row(i)=raw_quaternion_vec[i].coeffs().transpose();
//            //θ/2を格納する
//            raw_quaternion(i,3) = acos(angle_quaternion[i].w());
//            double r = sqrt(pow(angle_quaternion[i].x(),2.0)+pow(angle_quaternion[i].y(),2.0)+pow(angle_quaternion[i].z(),2.0));
//            if(r > 0.0001){
//                raw_quaternion.row(i).block(0,0,1,3) = angle_quaternion[i].coeffs().transpose().block(0,0,1,3)*asin(r)/r;
//            }else{
//                //不定形
//                raw_quaternion.row(i).block(0,0,1,3) = angle_quaternion[i].coeffs().transpose().block(0,0,1,3);
//            }
        }

        is_filtered = false;
    }*/

    vector<double> getRow(int r);

    const Eigen::MatrixXd &data();

    const Eigen::MatrixXd &toQuaternion();
//    const std::vector<Eigen::Quaterniond,Eigen::aligned_allocator<Eigen::Quaterniond>> &toQuaternion_vec();

    template <class T> void setFilterCoeff(T coeff){
        filter_coeff.resize(coeff.size(),1);
        for(int i=0,e=coeff.size();i<e;++i){
            filter_coeff(i,0) = coeff[i];
        }
    }

    const Eigen::MatrixXd &filteredData();

    static Eigen::VectorXd getKaiserWindow(uint32_t tap_length, uint32_t alpha, bool swap = true);
    static Eigen::VectorXd getKaiserWindowWithZeros(int32_t data_length, double alpha, int32_t window_length);



    static Eigen::VectorXcd getLPFFrequencyCoeff(uint32_t N, uint32_t alpha, double fs, double fc);
    static Eigen::VectorXcd getKaiserWindowWithZerosFrequencyCoeff(int32_t data_length, double alpha, int32_t window_length);


    static void Angle2CLerpedFrequency(double fs, double fc, const Eigen::MatrixXd &raw_angle, Eigen::MatrixXcd &freq_vectors);
    static void Frequency2Angle(Eigen::MatrixXcd &frequency_vector_, Eigen::MatrixXd &angle_);
    static void MatrixXcd2VectorXd(const Eigen::MatrixXcd &src, Eigen::VectorXd &dst);
    static void VectorXd2MatrixXcd(const Eigen::VectorXd &src, Eigen::MatrixXcd &dst);

    const Eigen::MatrixXd &filteredDataDFT(double fs, double fc);
    const Eigen::MatrixXd &filteredDataDFTTimeDomainOptimize(double fs, double fc, const Eigen::MatrixXd &coeff);
    Eigen::MatrixXd &filteredDataDFT();
    Eigen::MatrixXd &filteredQuaternion(uint32_t alpha, double fs, double fc);
    Eigen::Quaternion<double> toRawQuaternion(uint32_t frame);
    Eigen::Quaternion<double> toFilteredQuaternion(uint32_t frame);
    Eigen::Quaternion<double> toDiffQuaternion(uint32_t frame);
    Eigen::Quaternion<double> toDiffQuaternion2(uint32_t frame);



    /**
      * @brief cos関数で2点をなめらかに補完する関数
      * @retval numステップで補完された行列
      **/
    static Eigen::MatrixXd CLerp(Eigen::MatrixXd start, Eigen::MatrixXd end, int32_t num);

    /**
     * @brief 回転を表すクォータニオンを生成する関数
     **/
    static Eigen::Quaternion<double> RotationQuaternion(double theta, Eigen::Vector3d n);

    /**
     * @brief 微小回転を表す回転ベクトルから四元数を作る関数
     **/

    static Eigen::Quaternion<double> RotationQuaternion(Eigen::Vector3d w);

    /**
     * @param 回転を表すクォータニオンをシングルローテーションをあらわすベクトルへ変換
     **/
    template <typename T_num> static Eigen::Vector3d Quaternion2Vector(Eigen::Quaternion<T_num> q){
        double denom = sqrt(1-q.w()*q.w());
        if(abs(denom)<EPS){//まったく回転しない時は０割になるので、場合分けする//TODO:
            return Eigen::Vector3d(0,0,0);//return zero vector
        }
        return Eigen::Vector3d(q.x(),q.y(),q.z())*2.0*atan2(denom,q.w())/denom;
    }



    /**
     * @param シングルローテーションを表すベクトルを回転を表すクォータニオンへ変換
     **/
    template <typename T_num> static Eigen::Quaternion<T_num> Vector2Quaternion(Eigen::Vector3d w){
        double theta = w.norm();//sqrt(w[0]*w[0]+w[1]*w[1]+w[2]*w[2]);//回転角度を計算、normと等しい
        //0割を回避するためにマクローリン展開
        if(theta > EPS){
            Eigen::Vector3d n = w.normalized();//w * (1.0/theta);//単位ベクトルに変換
            //            double sin_theta_2 = sin(theta*0.5);
            //            return Eigen::Quaternion<T_num>(cos(theta*0.5),n[0]*sin_theta_2,n[1]*sin_theta_2,n[2]*sin_theta_2);
            Eigen::VectorXd n_sin_theta_2 = n * sin(theta*0.5);
            return Eigen::Quaternion<T_num>(cos(theta*0.5),n_sin_theta_2[0],n_sin_theta_2[1],n_sin_theta_2[2]);
        }else{
            return Eigen::Quaternion<T_num>(1.0,0.5*w[0],0.5*w[1],0.5*w[2]);
        }
    }

    /**
     * @brief AngleVectorをQuarternionに変換します。
     */
//    template <typename T_num> static Eigen::Quaternion<T_num> Vector2Quaternion(Eigen::VectorXd &angle, int32_t row){
//        return Vector2Quaternion<T_num>(angle.row(row).transpose());
//    }

//    Eigen::Quaternion<T_num> toDiffQuaternion()

    /**
     * @param 回転を表すクォータニオンをシングルローテーションを表すベクトルへ変換。前回算出したベクトルを引数として受け取ることで、アンラッピングする。
     * */
//    template <typename T_num> static Eigen::Vector3d Quaternion2Vector(Eigen::Quaternion<T_num> q, Eigen::Vector3d &prev){
//        double denom = sqrt(1-q.w()*q.w());
//        if(denom==0.0){//まったく回転しない時は０割になるので、場合分けする
//            return Eigen::Vector3d(0,0,0);//return zero vector
//        }
//        double theta = 2.0 * atan2(denom,q.w());
//        double prev_theta = prev.norm()/2;
//        double diff = theta - prev_theta;
//        theta -= 2.0*M_PI*(double)(static_cast<int>(diff/(2.0*M_PI)));//マイナスの符号に注意
//        //~ printf("Theta_2:%4.3f sc:%d\n",theta_2,static_cast<int>(diff/(2.0*M_PI)));
//        if(static_cast<int>(diff/(2.0*M_PI))!=0){
//            printf("\n###########Unwrapping %d\n",static_cast<int>(diff/(2.0*M_PI)));
//        }
////        std::cout <<  ", q:" << (Eigen::Vector3d(q.x(),q.y(),q.z())*2.0*theta_2/denom).transpose() << std::endl;
//        Eigen::Vector3d n(q.x(),q.y(),q.z());
//        n /= n.norm();
//        std::cout <<  "theta:" << theta << ", q:" << Eigen::Vector4d(q.w(),q.x(),q.y(),q.z()).transpose() <<
//                      " denom:" << denom <<
//                      " n:" << n.transpose() << std::endl;

//        return Eigen::Vector3d(q.x(),q.y(),q.z())*theta/denom;
//    }

    /**
     * @param 回転を表すクォータニオンから回転を表す行列を生成
     **/
    template <typename T_num> static void Quaternion2Matrix(Eigen::Quaternion<T_num> &q, Eigen::MatrixXd &det){
        det = q.matrix();
    }

    /** @brief 補正前の画像座標から、補正後のポリゴンの頂点を作成
     * @param [in]	Qa	ジャイロの角速度から計算したカメラの方向を表す回転クウォータニオン時系列データ、参照渡し
     * @param [in]	Qf	LPFを掛けて平滑化した回転クウォータニオンの時系列データ、参照渡し
     * @param [in]	m	画面の縦の分割数[ ]
     * @param [in]	n	画面の横の分割数[ ]
     * @param [in]	IK	"逆"歪係数(k1,k2,p1,p2)
     * @param [in]	matIntrinsic	カメラ行列(fx,fy,cx,cy) [pixel]
     * @param [in]	imageSize	フレーム画像のサイズ[pixel]
     * @param [in]  adjustmentQuaternion 画面方向を微調整するクォータニオン[rad]
     * @param [out]	vecPorigonn_uv	OpenGLのポリゴン座標(u',v')座標(-1~1)の組、歪補正後の画面を分割した時の一つ一つのポリゴンの頂点の組
     * @param [in]	zoom	倍率[]。拡大縮小しないなら1を指定すること。省略可
     * @retval true:成功 false:折り返し発生で失敗
     **/
    template <typename _Tp, typename _Tx> bool getDistortUnrollingContour(
            Eigen::Quaternion<_Tp> &prevAngleQuaternion,
            Eigen::Quaternion<_Tp> &currAngleQuaternion,
            Eigen::Quaternion<_Tp> &nextAngleQuaternion,
//            uint32_t division_x,
//            uint32_t division_y,
//            double TRollingShutter,
//            Eigen::MatrixXd &IK,
//            Eigen::MatrixXd &matIntrinsic,
//            uint32_t image_width,
//            uint32_t image_height,
            std::vector<_Tx> &vecPorigonn_uv
//            double zoom
            ){

        bool retval = true;


        //手順
        //1.補正前画像を分割した時の分割点の座標(pixel)を計算
        //2.1の座標を入力として、各行毎のW(t1,t2)を計算
        //3.補正後の画像上のポリゴン座標(pixel)を計算、歪み補正も含める

        double fx = matIntrinsic(0, 0);
        double fy = matIntrinsic(1, 1);
        double cx = matIntrinsic(0, 2);
        double cy = matIntrinsic(1, 2);
        double k1 = IK(0,0);
        double k2 = IK(0,1);
        double p1 = IK(0,2);
        double p2 = IK(0,3);

        vecPorigonn_uv.clear();

        //top
        //    for(int j=0;j<=division_y;++j){
        {
            int j=0;
            //W(t1,t2)を計算
            Eigen::MatrixXd R;
            //1
            double v = (double)j/division_y*image_height;

            double exposureTimingInEachRow = TRollingShutter*v/image_height;	//ローリングシャッターの読み込みを考慮した各行毎のサンプル時間[sec]

            Eigen::Quaternion<double> slerpedAngleQuaternion;
            if(exposureTimingInEachRow >= 0){
                slerpedAngleQuaternion = currAngleQuaternion.slerp(exposureTimingInEachRow,nextAngleQuaternion);
            }else{
                slerpedAngleQuaternion = prevAngleQuaternion.slerp(1.0+exposureTimingInEachRow,currAngleQuaternion);
            }
            Quaternion2Matrix(slerpedAngleQuaternion,R);
            for(int i=0;i<=division_x;++i){
                double u = (double)i/division_x*image_width;
                //後々の行列演算に備えて、画像上の座標を同次座標で表現しておく。(x座標、y座標,1)T
                Eigen::Vector3d p;
                p << (u- cx)/fx, (v - cy)/fy, 1.0;	//1のポリゴン座標に、K^-1を掛けた結果の３x１行列
                //2
                Eigen::MatrixXd XYW = R * p;//inv()なし

                if(XYW(2,0) < 0.0){
                    retval = false;
                }

                double x1 = XYW(0, 0)/XYW(2, 0);
                double y1 = XYW(1, 0)/XYW(2, 0);

                double r = sqrt(x1*x1+y1*y1);

                double x2 = x1*(1.0+k1*r*r+k2*r*r*r*r)+2.0*p1*x1*y1+p2*(r*r+2.0*x1*x1);
                double y2 = y1*(1.0+k1*r*r+k2*r*r*r*r)+p1*(r*r+2.0*y1*y1)+2.0*p2*x1*y1;
                //変な折り返しを防止
                if((pow(x2-x1,2)>1.0)||(pow(y2-y1,2)>1.0)){
                    //                printf("折り返し防止\r\n");
                    x2 = x1;
                    y2 = y1;
                }
                vecPorigonn_uv.push_back(x2*fx*zoom/image_width*2.0);
                vecPorigonn_uv.push_back(y2*fy*zoom/image_height*2.0);
            }
        }

        //middle
        for(int j=1;j<division_y;++j){
            //W(t1,t2)を計算
            Eigen::MatrixXd R;
            //1
            double v = (double)j/division_y*image_height;

            double exposureTimingInEachRow = TRollingShutter*v/image_height;	//ローリングシャッターの読み込みを考慮した各行毎のサンプル時間[sec]

            Eigen::Quaternion<double> slerpedAngleQuaternion;
            if(exposureTimingInEachRow >= 0){
                slerpedAngleQuaternion = currAngleQuaternion.slerp(exposureTimingInEachRow,nextAngleQuaternion);
            }else{
                slerpedAngleQuaternion = prevAngleQuaternion.slerp(1.0+exposureTimingInEachRow,currAngleQuaternion);
            }
            Quaternion2Matrix(slerpedAngleQuaternion,R);
            for(int i=0;i<=division_x;i+=division_x){
                double u = (double)i/division_x*image_width;
                //後々の行列演算に備えて、画像上の座標を同次座標で表現しておく。(x座標、y座標,1)T
                Eigen::Vector3d p;
                p << (u- cx)/fx, (v - cy)/fy, 1.0;	//1のポリゴン座標に、K^-1を掛けた結果の３x１行列
                //2
                Eigen::MatrixXd XYW = R * p;//inv()なし

                if(XYW(2,0) < 0.0){
                    retval = false;
                }

                double x1 = XYW(0, 0)/XYW(2, 0);
                double y1 = XYW(1, 0)/XYW(2, 0);

                double r = sqrt(x1*x1+y1*y1);

                double x2 = x1*(1.0+k1*r*r+k2*r*r*r*r)+2.0*p1*x1*y1+p2*(r*r+2.0*x1*x1);
                double y2 = y1*(1.0+k1*r*r+k2*r*r*r*r)+p1*(r*r+2.0*y1*y1)+2.0*p2*x1*y1;
                //変な折り返しを防止
                if((pow(x2-x1,2)>1.0)||(pow(y2-y1,2)>1.0)){
                    //                printf("折り返し防止\r\n");
                    x2 = x1;
                    y2 = y1;
                }
                vecPorigonn_uv.push_back(x2*fx*zoom/image_width*2.0);
                vecPorigonn_uv.push_back(y2*fy*zoom/image_height*2.0);
            }
        }

        //bottom
        {
            int j=division_y;
            //W(t1,t2)を計算
            Eigen::MatrixXd R;
            //1
            double v = (double)j/division_y*image_height;

            double exposureTimingInEachRow = TRollingShutter*v/image_height;	//ローリングシャッターの読み込みを考慮した各行毎のサンプル時間[sec]

            Eigen::Quaternion<double> slerpedAngleQuaternion;
            if(exposureTimingInEachRow >= 0){
                slerpedAngleQuaternion = currAngleQuaternion.slerp(exposureTimingInEachRow,nextAngleQuaternion);
            }else{
                slerpedAngleQuaternion = prevAngleQuaternion.slerp(1.0+exposureTimingInEachRow,currAngleQuaternion);
            }
            Quaternion2Matrix(slerpedAngleQuaternion,R);
            for(int i=0;i<=division_x;++i){
                double u = (double)i/division_x*image_width;
                //後々の行列演算に備えて、画像上の座標を同次座標で表現しておく。(x座標、y座標,1)T
                Eigen::Vector3d p;
                p  << (u- cx)/fx, (v - cy)/fy, 1.0;	//1のポリゴン座標に、K^-1を掛けた結果の３x１行列
                //2
                Eigen::MatrixXd XYW = R * p;//inv()なし

                if(XYW(2,0) < 0.0){
                    retval = false;
                }

                double x1 = XYW(0, 0)/XYW(2, 0);
                double y1 = XYW(1, 0)/XYW(2, 0);

                double r = sqrt(x1*x1+y1*y1);

                double x2 = x1*(1.0+k1*r*r+k2*r*r*r*r)+2.0*p1*x1*y1+p2*(r*r+2.0*x1*x1);
                double y2 = y1*(1.0+k1*r*r+k2*r*r*r*r)+p1*(r*r+2.0*y1*y1)+2.0*p2*x1*y1;
                //変な折り返しを防止
                if((pow(x2-x1,2)>1.0)||(pow(y2-y1,2)>1.0)){
                    //                printf("折り返し防止\r\n");
                    x2 = x1;
                    y2 = y1;
                }
                vecPorigonn_uv.push_back(x2*fx*zoom/image_width*2.0);
                vecPorigonn_uv.push_back(y2*fy*zoom/image_height*2.0);
            }
        }

        return retval;
    }

    /** @brief 補正前の画像座標から、補正後のポリゴンの頂点を作成
     * @param [in] frame フレーム番号
     * @param [out]	vecPorigonn_uv	OpenGLのポリゴン座標(u',v')座標(-1~1)の組、歪補正後の画面を分割した時の一つ一つのポリゴンの頂点の組
     * @retval true:成功 false:折り返し発生で失敗
     **/
    template <typename _Tx> bool getDistortUnrollingMap(
            int32_t frame,
            std::vector<_Tx> &vecPorigonn_uv
            ){


        Eigen::Quaternion<double> prevAngleQuaternion = this->toDiffQuaternion(frame);
        Eigen::Quaternion<double> currAngleQuaternion = this->toDiffQuaternion(frame+1);//インデックスの付け方が気持ち悪い。TODO:何とかする
        Eigen::Quaternion<double> nextAngleQuaternion = this->toDiffQuaternion(frame+2);
        bool retval = true;


        //手順
        //1.補正前画像を分割した時の分割点の座標(pixel)を計算
        //2.1の座標を入力として、各行毎のW(t1,t2)を計算
        //3.補正後の画像上のポリゴン座標(pixel)を計算、歪み補正も含める

        double fx = matIntrinsic(0, 0);
        double fy = matIntrinsic(1, 1);
        double cx = matIntrinsic(0, 2);
        double cy = matIntrinsic(1, 2);
        double k1 = IK(0,0);
        double k2 = IK(0,1);
        double p1 = IK(0,2);
        double p2 = IK(0,3);

        vecPorigonn_uv.clear();
        Eigen::MatrixXd map_x = Eigen::MatrixXd::Zero(division_y+1,division_x+1);
        Eigen::MatrixXd map_y = Eigen::MatrixXd::Zero(division_y+1,division_x+1);
        for(int j=0;j<=division_y;++j)
        {
            //W(t1,t2)を計算
            Eigen::MatrixXd R;
            //1
            double v = (double)j/division_y*image_height;

            double exposureTimingInEachRow = TRollingShutter*v/image_height;	//ローリングシャッターの読み込みを考慮した各行毎のサンプル時間[sec]

            Eigen::Quaternion<double> slerpedAngleQuaternion;
            if(exposureTimingInEachRow >= 0){
                slerpedAngleQuaternion = currAngleQuaternion.slerp(exposureTimingInEachRow,nextAngleQuaternion);
            }else{
                slerpedAngleQuaternion = prevAngleQuaternion.slerp(1.0+exposureTimingInEachRow,currAngleQuaternion);
            }
            Quaternion2Matrix(slerpedAngleQuaternion,R);
            for(int i=0;i<=division_x;++i){
                double u = (double)i/division_x*image_width;
                //後々の行列演算に備えて、画像上の座標を同次座標で表現しておく。(x座標、y座標,1)T
                Eigen::Vector3d p;
                p << (u- cx)/fx, (v - cy)/fy, 1.0;	//1のポリゴン座標に、K^-1を掛けた結果の３x１行列
                //2
                Eigen::MatrixXd XYW = R * p;//inv()なし

                if(XYW(2,0) < 0.0){
                    retval = false;
                }

                double x1 = XYW(0, 0)/XYW(2, 0);
                double y1 = XYW(1, 0)/XYW(2, 0);

                double r = sqrt(x1*x1+y1*y1);

                double x2 = x1*(1.0+k1*r*r+k2*r*r*r*r)+2.0*p1*x1*y1+p2*(r*r+2.0*x1*x1);
                double y2 = y1*(1.0+k1*r*r+k2*r*r*r*r)+p1*(r*r+2.0*y1*y1)+2.0*p2*x1*y1;
                //変な折り返しを防止
                if((pow(x2-x1,2)>1.0)||(pow(y2-y1,2)>1.0)){
                    //                printf("折り返し防止\r\n");
                    x2 = x1;
                    y2 = y1;
                }
//                vecPorigonn_uv.push_back(x2*fx*zoom/image_width*2.0);
//                vecPorigonn_uv.push_back(y2*fy*zoom/image_height*2.0);
                map_x(j,i) = x2*fx*zoom/image_width*2.0;
                map_y(j,i) = y2*fy*zoom/image_height*2.0;

                }
        }

        //3.ポリゴン座標をOpenGLの関数に渡すために順番を書き換える
        vecPorigonn_uv.clear();
        for(int j=0;j<division_y;++j){//jは終了の判定が"<"であることに注意
            for(int i=0;i<division_x;++i){
                //GL_TRIANGLESでGL側へ送信するポリゴンの頂点座標を準備
                vecPorigonn_uv.push_back(map_x(j,i));//x座標
                vecPorigonn_uv.push_back(map_y(j,i));//y座標
                vecPorigonn_uv.push_back(map_x(j,i+1));//x座標
                vecPorigonn_uv.push_back(map_y(j,i+1));//y座標
                vecPorigonn_uv.push_back(map_x(j+1,i));//x座標
                vecPorigonn_uv.push_back(map_y(j+1,i));//y座標

                vecPorigonn_uv.push_back(map_x(j+1,i));//x座標
                vecPorigonn_uv.push_back(map_y(j+1,i));//y座標
                vecPorigonn_uv.push_back(map_x(j,i+1));//x座標
                vecPorigonn_uv.push_back(map_y(j,i+1));//y座標
                vecPorigonn_uv.push_back(map_x(j+1,i+1));//x座標
                vecPorigonn_uv.push_back(map_y(j+1,i+1));//y座標


            }
        }

        return retval;
    }

    template <typename _Tx> bool getDistortUnrollingMapQuaternion(
            int32_t frame,
            std::vector<_Tx> &vecPorigonn_uv
            ){


        Eigen::Quaternion<double> prevAngleQuaternion = this->toDiffQuaternion2(frame);
        Eigen::Quaternion<double> currAngleQuaternion = this->toDiffQuaternion2(frame+1);//インデックスの付け方が気持ち悪い。TODO:何とかする
        Eigen::Quaternion<double> nextAngleQuaternion = this->toDiffQuaternion2(frame+2);
        bool retval = true;


        //手順
        //1.補正前画像を分割した時の分割点の座標(pixel)を計算
        //2.1の座標を入力として、各行毎のW(t1,t2)を計算
        //3.補正後の画像上のポリゴン座標(pixel)を計算、歪み補正も含める

        double fx = matIntrinsic(0, 0);
        double fy = matIntrinsic(1, 1);
        double cx = matIntrinsic(0, 2);
        double cy = matIntrinsic(1, 2);
        double k1 = IK(0,0);
        double k2 = IK(0,1);
        double p1 = IK(0,2);
        double p2 = IK(0,3);

        vecPorigonn_uv.clear();
        Eigen::MatrixXd map_x = Eigen::MatrixXd::Zero(division_y+1,division_x+1);
        Eigen::MatrixXd map_y = Eigen::MatrixXd::Zero(division_y+1,division_x+1);
        for(int j=0;j<=division_y;++j)
        {
            //W(t1,t2)を計算
            Eigen::MatrixXd R;
            //1
            double v = (double)j/division_y*image_height;

            double exposureTimingInEachRow = TRollingShutter*v/image_height;	//ローリングシャッターの読み込みを考慮した各行毎のサンプル時間[sec]

            Eigen::Quaternion<double> slerpedAngleQuaternion;
            if(exposureTimingInEachRow >= 0){
                slerpedAngleQuaternion = currAngleQuaternion.slerp(exposureTimingInEachRow,nextAngleQuaternion);
            }else{
                slerpedAngleQuaternion = prevAngleQuaternion.slerp(1.0+exposureTimingInEachRow,currAngleQuaternion);
            }
            Quaternion2Matrix(slerpedAngleQuaternion,R);
            for(int i=0;i<=division_x;++i){
                double u = (double)i/division_x*image_width;
                //後々の行列演算に備えて、画像上の座標を同次座標で表現しておく。(x座標、y座標,1)T
                Eigen::Vector3d p;
                p << (u- cx)/fx, (v - cy)/fy, 1.0;	//1のポリゴン座標に、K^-1を掛けた結果の３x１行列
                //2
                Eigen::MatrixXd XYW = R * p;//inv()なし

                if(XYW(2,0) < 0.0){
                    retval = false;
                }

                double x1 = XYW(0, 0)/XYW(2, 0);
                double y1 = XYW(1, 0)/XYW(2, 0);

                double r = sqrt(x1*x1+y1*y1);

                double x2 = x1*(1.0+k1*r*r+k2*r*r*r*r)+2.0*p1*x1*y1+p2*(r*r+2.0*x1*x1);
                double y2 = y1*(1.0+k1*r*r+k2*r*r*r*r)+p1*(r*r+2.0*y1*y1)+2.0*p2*x1*y1;
                //変な折り返しを防止
                if((pow(x2-x1,2)>1.0)||(pow(y2-y1,2)>1.0)){
                    //                printf("折り返し防止\r\n");
                    x2 = x1;
                    y2 = y1;
                }
//                vecPorigonn_uv.push_back(x2*fx*zoom/image_width*2.0);
//                vecPorigonn_uv.push_back(y2*fy*zoom/image_height*2.0);
                map_x(j,i) = x2*fx*zoom/image_width*2.0;
                map_y(j,i) = y2*fy*zoom/image_height*2.0;

                }
        }

        //3.ポリゴン座標をOpenGLの関数に渡すために順番を書き換える
        vecPorigonn_uv.clear();
        for(int j=0;j<division_y;++j){//jは終了の判定が"<"であることに注意
            for(int i=0;i<division_x;++i){
                //GL_TRIANGLESでGL側へ送信するポリゴンの頂点座標を準備
                vecPorigonn_uv.push_back(map_x(j,i));//x座標
                vecPorigonn_uv.push_back(map_y(j,i));//y座標
                vecPorigonn_uv.push_back(map_x(j,i+1));//x座標
                vecPorigonn_uv.push_back(map_y(j,i+1));//y座標
                vecPorigonn_uv.push_back(map_x(j+1,i));//x座標
                vecPorigonn_uv.push_back(map_y(j+1,i));//y座標

                vecPorigonn_uv.push_back(map_x(j+1,i));//x座標
                vecPorigonn_uv.push_back(map_y(j+1,i));//y座標
                vecPorigonn_uv.push_back(map_x(j,i+1));//x座標
                vecPorigonn_uv.push_back(map_y(j,i+1));//y座標
                vecPorigonn_uv.push_back(map_x(j+1,i+1));//x座標
                vecPorigonn_uv.push_back(map_y(j+1,i+1));//y座標


            }
        }

        return retval;
    }

    /**
     * @brief ワープした時に欠けがないかチェックします
     * @retval false:欠けあり true:ワープが良好
     **/
    template <typename _Tp> static bool check_warp(vector<_Tp> &contour){
        for(int i=0;i<contour.size();i+=2){
            if((abs(contour[i]) < 1.0)&&(abs(contour[i+1]) < 1.0)){
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 画面の欠けを生み出している回転ベクトルのオーバーした長さを返す
     **/
    Eigen::VectorXd getRollingVectorError();

    /**
      * @brief 同期が取れている角速度を出力
      **/
    Eigen::Vector3d angularVelocitySync(/*std::vector<Eigen::Vector3d,Eigen::aligned_allocator<Eigen::Vector3d>> &angularVelocityIn60Hz,
                                        double T_video,
                                        double T_av,
                                        double frame_offset,*/
                                        int32_t frame);/*{
        double dframe = (frame + frame_offset) * T_video / T_angular_velocity;
        int i = floor(dframe);
        double decimalPart = dframe - (double)i;
        //領域外にはみ出した時は、末端の値で埋める
        if(i<0){
            return angular_velocity[0];
        }else if(angular_velocity.size()<=(i+1)){
            return angular_velocity.back();
        }else{
            return angular_velocity[i]*(1.0-decimalPart)+angular_velocity[i+1]*decimalPart;
        }
    }*/

    /**
     * @brief CSVファイルを読み込んで配列を返す関数
     **/
    template <typename _Tp, typename _Alloc = std::allocator<_Tp>> static void ReadCSV(std::vector<_Tp,_Alloc> &w, const char* filename){
            std::ifstream ifs(filename);//CSVファイルを開く
            if(!ifs){
                    std::cout << "エラー：CSVファイルが見つかりません\n" << std::endl;
                    return;
            }

            std::string str;
            w.clear();
            _Tp numl;
            while(getline(ifs,str)){//1行ずつ読み込む
                    std::string token;
                    std::istringstream stream(str);
                    try{
                        int i=0;
                        while(getline(stream, token, ',')){
                            double temp = stof(token);
                            numl[i++] = (double)temp;//値を保存
                        }

                    }catch(...){
                        //読み込みに失敗したら次の行へすすむ
                        continue;
                    }
                    w.push_back(numl);
            }

            printf("size of w is %ld\n",w.size());
            return;
    }

    int init_opengl(cv::Size textureSize);
    int stop_opengl();
    int spin_once(int frame,cv::VideoCapture &capture,cv::Mat &simg);
    bool ok();
private:
    enum KEY {
        KEY_SIDEBYSIDE = '1',
        KEY_ORIGINAL = '2',
        KEY_STABILIZED = '3',
        KEY_QUIT = 'q'
    };

    Eigen::MatrixXd raw_angle;
    Eigen::MatrixXd raw_quaternion;
    std::vector<Eigen::Quaterniond,Eigen::aligned_allocator<Eigen::Quaterniond>> raw_quaternion_with_margin;
    Eigen::MatrixXd filtered_angle;
    Eigen::MatrixXd filtered_quaternion;
    Eigen::VectorXd filter_coeff;
    bool is_filtered;
    bool quaternion_is_filtered=false;
    int32_t division_x = 9;
    int32_t division_y = 9;
    double TRollingShutter = 0.0;
    Eigen::MatrixXd IK = Eigen::MatrixXd::Zero(1,4);
    Eigen::MatrixXd matIntrinsic = Eigen::MatrixXd::Identity(3,3);
    int32_t image_width=1920;
    int32_t image_height=1080;
    double zoom=1.0;

    std::vector<Eigen::Vector3d,Eigen::aligned_allocator<Eigen::Vector3d>> angular_velocity;
    double T_video=1.0/30.0;
    double T_angular_velocity=1.0/60.0;
    double frame_offset=0;
    int32_t video_frames=0;
    int32_t filter_tap_length=0;

    char key = '1';

    //OpenGL
    cv::Size textureSize;
    cv::Mat buff;
    GLuint vertexbuffer;
    GLuint uvbuffer;
    GLuint programID;
    GLuint TextureID;
    GLuint VertexArrayID;
    std::vector<GLfloat> vecVtx;					//頂点座標
    GLuint FramebufferName = 0;
    GLuint MatrixID;
    cv::Mat img;
    GLuint textureID_0;
    GLuint nFxyID;
    GLuint nCxyID;
    GLuint distCoeffID;
//    bool outputStabilizedVideo = false;

};

#endif // VSP_H
