#include <stdio.h>
#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <iomanip>
//Quartanion
#include <boost/math/quaternion.hpp>
#include "stabilize.h"
#include "distortion.h"
using namespace boost::math;

// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
//GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>

#include "calcShift.hpp"
#include <chrono>

#include "mINIRead.hpp"

//Numetric optimization
#include "levenbergMarquardt.hpp"

#include "settings.h"
#include "seekablevideocapture.h"

#include <mutex>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include "visualizer.h"

#include <Eigen/Dense>
#include <unsupported/Eigen/FFT>
#include "vsp.h"
#include "frequency_domain_optimization.hpp"
using namespace std;

struct videoBufferAndWriter{
    std::mutex mtx;
    volatile bool isWriting;
    std::deque<cv::Mat> images;
    //    std::string videoPass;
    cv::VideoWriter vw;
};

struct strMultiThreadVideoCapture{
    std::mutex mtx;
    std::deque<cv::Mat> images;
    int32_t maxLength;
    cv::VideoCapture vc;
};

videoBufferAndWriter buffer;
strMultiThreadVideoCapture mtvc;


void videoWriterProcess(){
    cv::Mat _buf;
    while(1){//繰り返し書き込み
        {
            std::lock_guard<std::mutex> lock(buffer.mtx);
            //bufferにデータがあるか確認
            if(buffer.images.size()!=0){
                //先頭をコピー
                _buf = buffer.images.front().clone();
                //先頭を削除
                buffer.images.pop_front();
            }else if(!buffer.isWriting){
                return;
            }
        }
        //mutexがunlockされたあとにゆっくりvideoWriterに書き込み
        if(!_buf.empty()){
            buffer.vw << _buf;
            _buf = cv::Mat();
        }else{
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}



void show_correlation(std::vector<Eigen::Vector3d,Eigen::aligned_allocator<Eigen::Vector3d>> &angularVelocityIn60Hz, std::vector<cv::Vec3d> estimatedAngularVelocity, double Tvideo, double Tav, double frame){

    auto angularVelocity = [&angularVelocityIn60Hz, Tvideo, Tav](double frame_){
        double dframe = frame_ * Tvideo / Tav;
        int i = floor(dframe);
        double decimalPart = dframe - (double)i;
        return angularVelocityIn60Hz[i]*(1.0-decimalPart)+angularVelocityIn60Hz[i+1]*decimalPart;
    };

    for(double d=-0.5;d<=0.5;d+=0.01)
    {

        double sum = 0.0;
        for(int32_t i=0; i<estimatedAngularVelocity.size();i++){
            sum +=   abs(angularVelocity(i+frame+d)[0]-estimatedAngularVelocity[i][0])
                    + abs(angularVelocity(i+frame+d)[1]-estimatedAngularVelocity[i][1])
                    + abs(angularVelocity(i+frame+d)[2]-estimatedAngularVelocity[i][2]);
        }
        cout << "position"<<frame+d<<" minimum correlationCoefficients:" << sum << endl;
    }
}

int main(int argc, char** argv){
    bool debug_signal_processing = false;

    //テクスチャ座標の準備
    int32_t division_x = 9; //画面の横の分割数
    int32_t division_y = 9; //画面の縦の分割数
    cv::Size textureSize = cv::Size(2048,2048);

    //画面の補正量
    float vAngle = 0.f;
    float hAngle = 0.f;
    float zoomRatio = 1.f;
    double rollingShutterDuration = 0; //rolling shutter duration [frame]
//    int32_t lowPassFilterStrength = 3;
    //引数の確認
    char *videoPass = NULL;
    char *csvPass = NULL;
    //    char *outputPass = NULL;
    bool outputStabilizedVideo = false;
    int opt;
    while((opt = getopt(argc, argv, "i:c:o::d::v:h:z:r:f:")) != -1){
        string value1 ;//= optarg;
        switch (opt) {
        case 'i':       //input video file pass
            videoPass = optarg;
            break;
        case 'c':       //input angular velocity csv file pass
            csvPass = optarg;
            break;
        case 'o':       //output
            outputStabilizedVideo = true;
            break;
        case 'd':
            debug_signal_processing = true;
            break;
        case 'v':       //vertical position adjustment [rad], default 0
            value1 = optarg;
            vAngle = std::stof(value1);
            break;
        case 'h':       //horizontal position adjustment [rad], default 0
            value1 = optarg;
            hAngle = std::stof(value1);
            break;
        case 'z':       //zoom ratio, dafault 1.0
            value1 = optarg;
            zoomRatio = std::stof(value1);
            break;
        case 'r':       //rolling shutter duration [frame]. This should be between -1 and 1.
            value1 = optarg;
            rollingShutterDuration = std::stof(value1);
            if(rollingShutterDuration > 1.0){
                rollingShutterDuration = 1.0;
            }else if(rollingShutterDuration < -1.0){
                rollingShutterDuration = -1.0;
            }
            break;
        case 'f':       //Low pass filter strength of a camera shake reduction.
            //Larger is strong filter. This parameter must be integer.
            //Default 3.
            value1 = optarg;
//            lowPassFilterStrength = std::stoi(value1);
            std::cout << "Currently, -f option doesn't work. This function will be implemented in the future." << std::endl;
//            if(lowPassFilterStrength < 0){
//                cout << "Low pass filter strength must be greater than or equal to 0.\r\n" <<
//                        "It is set as 0 automatically." << endl;
//                lowPassFilterStrength = 0;
//            }else if(lowPassFilterStrength > 11){
//                cout << "Low pass filter strength must be less than 12.\r\n" <<
//                        "It is set as 11 automatically." << endl;
//                lowPassFilterStrength = 1;
//            }
            break;
        default :
            printf(     "virtualGimbal\r\n"
                        "Hyper fast video stabilizer\r\n\r\n"
                        "usage: virtualGimbal [-i video] [-f angularVelocity] [[output option] -o] [options]\r\n"
                        );
            return 1;
        }
    }

    //動画からオプティカルフローを計算する
    auto t1 = std::chrono::system_clock::now() ;
    std::vector<cv::Vec3d> opticShift = CalcShiftFromVideo(videoPass,SYNC_LENGTH);//ビデオからオプティカルフローを用いてシフト量を算出
    auto t2 = std::chrono::system_clock::now() ;
    // 処理の経過時間
    auto elapsed = t2 - t1 ;
    std::cout << "Elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << " ms\n";
    //show(opticShift,"opticShift");
    cv::VideoCapture *Capture = new cv::VideoCapture(videoPass);//動画をオープン
    assert(Capture->isOpened());
    cv::Size imageSize = cv::Size(Capture->get(CV_CAP_PROP_FRAME_WIDTH),Capture->get(CV_CAP_PROP_FRAME_HEIGHT));//解像度を読む
    std::cout << "Static size:" << textureSize << std::endl;

    //テキスチャーのサイズをここで計算する
    textureSize.width = pow(2.0,ceil(log(imageSize.width)/log(2.0)));
    textureSize.height = pow(2.0,ceil(log(imageSize.height)/log(2.0)));
    std::cout << "Adaptive size:" << textureSize << std::endl;


    double Tvideo = 1.0/Capture->get(CV_CAP_PROP_FPS);
    std::cout << "resolution" << imageSize << std::endl;
    std::cout << "samplingPeriod" << Tvideo << std::endl;

    //内部パラメータを読み込み
    cv::Mat matIntrinsic;
    ReadIntrinsicsParams("intrinsic.txt",matIntrinsic);
    std::cout << "Camera matrix:\n" << matIntrinsic << "\n" <<  std::endl;
    double fx = matIntrinsic.at<double>(0,0);
    double fy = matIntrinsic.at<double>(1,1);
    double cx = matIntrinsic.at<double>(0,2);
    double cy = matIntrinsic.at<double>(1,2);

    //動画書き出しのマルチスレッド処理の準備
    buffer.isWriting = true;
    if(outputStabilizedVideo){
        std::string outputPass= videoPass;
        outputPass = outputPass + "_deblured.avi";
        buffer.vw = cv::VideoWriter(outputPass,CV_FOURCC('F', 'M', 'P', '4'),1/Tvideo,cv::Size(imageSize.width,imageSize.height),true);
        if(!buffer.vw.isOpened()){
            printf("Error:Can't Open Video Writer.");
            return -1;
        }
    }
    std::thread th1;
    if(outputStabilizedVideo){
        th1 = std::thread(videoWriterProcess);//スレッド起動
    }



    //歪パラメータの読み込み
    cv::Mat matDist;
    ReadDistortionParams("distortion.txt",matDist);
    std::cout << "Distortion Coeff:\n" << matDist << "\n" << std::endl;

    //逆歪パラメータの計算
    cv::Mat matInvDistort;
    calcDistortCoeff(matIntrinsic,matDist,imageSize,matInvDistort);

    //逆歪パラメータ表示
    if(PRINT_INV_DISTORT_COEFF){
        cout << "distCoeff:" << matDist << endl;
        cout << "invert distCoeff:" << matInvDistort << endl;
    }


    //角速度データを読み込み

    std::vector<Eigen::Vector3d,Eigen::aligned_allocator<Eigen::Vector3d>> angular_velocity_from_csv;
    vsp::ReadCSV(angular_velocity_from_csv,csvPass);

    //ジャイロのDCオフセット（いわゆる温度ドリフトと等価）を計算。単純にフレームの平均値を計算
    if(SUBTRACT_OFFSET){
        Eigen::Vector3d dc(0,0,0);
        for(auto el:angular_velocity_from_csv){
            dc[0] += el[0];
            dc[1] += el[1];
            dc[2] += el[2];
        }
        dc[0]/=angular_velocity_from_csv.size();
        dc[1]/=angular_velocity_from_csv.size();
        dc[2]/=angular_velocity_from_csv.size();
        for(auto &el:angular_velocity_from_csv){
            el[0] -= dc[0];
            el[1] -= dc[1];
            el[2] -= dc[2];
        }
    }

    double Tav = 1/60.0;//Sampling period of angular velocity

    //動画のサンプリング周期に合わせて、角速度を得られるようにする関数を定義
    //線形補間
    auto angularVelocity = [&angular_velocity_from_csv, Tvideo, Tav](uint32_t frame){
        double dframe = frame * Tvideo / Tav;
        int i = floor(dframe);
        double decimalPart = dframe - (double)i;
        return angular_velocity_from_csv[i]*(1.0-decimalPart)+angular_velocity_from_csv[i+1]*decimalPart;
    };


    //動画のオプティカルフローと内部パラメータと解像度から角速度推定値を計算
    vector<cv::Vec3d> estimatedAngularVelocity;
    for(auto el:opticShift){
        estimatedAngularVelocity.push_back(cv::Vec3d(-atan(el[1]/fy),atan(el[0]/fx),el[2])/Tvideo*-1);
    }

    t1 = std::chrono::system_clock::now() ;

    int32_t angular_velocity_length_in_video = ceil(angular_velocity_from_csv.size() * Tav / Tvideo);
    int32_t lengthDiff = angular_velocity_length_in_video - estimatedAngularVelocity.size();
    cout << "lengthDiff:" << lengthDiff << endl;

    t2 = std::chrono::system_clock::now() ;

    //ここでEigenのMatrixでできるだけ計算するルーチンを追加してみる
    Eigen::MatrixXd angular_velocity_matrix             = Eigen::MatrixXd::Zero(ceil(angular_velocity_from_csv.size()*Tav /Tvideo),3);
    for(int32_t i=0;i<angular_velocity_length_in_video;++i){
//        angular_velocity_matrix.row(i) = Eigen::Map<Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>((double*)(&angularVelocity(i)[0]),1,3);
        angular_velocity_matrix.row(i) = angularVelocity(i).transpose();
    }
    Eigen::MatrixXd estimated_angular_velocity_matrix   = Eigen::Map<Matrix<double, Eigen::Dynamic, Eigen::Dynamic, RowMajor>>((double*)(estimatedAngularVelocity.data()),estimatedAngularVelocity.size(),3);
    vector<double> correlation_coefficients(lengthDiff);
    for(int32_t offset=0;offset<lengthDiff;++offset){
        correlation_coefficients[offset] = (angular_velocity_matrix.block(offset,0,estimated_angular_velocity_matrix.rows(),estimated_angular_velocity_matrix.cols())
                -estimated_angular_velocity_matrix).array().abs().sum();
    }
int32_t min_position = std::distance(correlation_coefficients.begin(),min_element(correlation_coefficients.begin(),correlation_coefficients.end()));


    auto t3 = std::chrono::system_clock::now() ;
    // 処理の経過時間
    elapsed = t3 - t2 ;
    std::cout << "Elapsed time@search minimum in new method: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << " ms" << std::endl;



    //最小値サブピクセル推定
    double subframeOffset;
    if(min_position == 0){	//位置が最初のフレームで一致している場合
        subframeOffset = 0.0;
    }else if(min_position == (lengthDiff-1)){//末尾
        subframeOffset = (double)(lengthDiff -1);
    }else{					//その他
        subframeOffset = -(correlation_coefficients[min_position+1]-correlation_coefficients[min_position-1])/(2*correlation_coefficients[min_position-1]-4*correlation_coefficients[min_position]+2*correlation_coefficients[min_position+1]);
    }
    cout << "min_position" << min_position << endl;
//    cout << "minPosition" << minPosition << endl;
    cout << "subframe minposition :" << min_position+subframeOffset << endl;

    if(debug_signal_processing) show_correlation(angular_velocity_from_csv,estimatedAngularVelocity, Tvideo,Tav,min_position+subframeOffset);

    auto convCVMat2EigenMat = [](cv::Mat &src){
        assert(src.type()==CV_64FC1);
      Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> retval;
      retval.resize(src.rows,src.cols);
      memcpy(retval.data(),src.data,src.size().area()*sizeof(double));
      return retval;
    };


    //EigenによるDFT LPFのテスト
    if(debug_signal_processing){

        vsp v2(/*angleQuaternion_vsp2,*/
               division_x,
               division_y,
               rollingShutterDuration,
               convCVMat2EigenMat(matInvDistort),
               convCVMat2EigenMat(matIntrinsic),
               imageSize.width,
               imageSize.height,
               (double)zoomRatio,
               angular_velocity_from_csv,
               Tvideo,
               Tav,
               min_position + subframeOffset,
               (int32_t)(Capture->get(CV_CAP_PROP_FRAME_COUNT)));
        std::vector<string> legends_quaternion = {"x","y","z","w"};
        vgp::plot(v2.toQuaternion(),"Raw Quaternion",legends_quaternion);
        vgp::plot(v2.filteredQuaternion(100,Capture->get(CV_CAP_PROP_FPS),1.0),"Filtered Quaternion",legends_quaternion);

        std::vector<string> legends = {"x","y","z"};
        vgp::plot(v2.data(),"Raw DFT",legends);

        //平滑化を試す
        //時間波形補正実装済み平滑化
        Eigen::MatrixXd coeffs = Eigen::MatrixXd::Ones(v2.data().rows()*1.0/Capture->get(CV_CAP_PROP_FPS)+2,v2.data().cols());

        vgp::plot(v2.filteredDataDFTTimeDomainOptimize(Capture->get(CV_CAP_PROP_FPS),1.0,coeffs),"Filtered DFT DO",legends);
        //通常版平滑化
        vgp::plot(v2.filteredDataDFT(Capture->get(CV_CAP_PROP_FPS),1.0),"Filterd DFT",legends);



        //エラーを計算する。将来的に所望の特性の波形が得られるまで、DFTした複素配列の値をいじることになる？？？どうやって複素配列にアクセスする？
        Eigen::VectorXd errors = v2.getRollingVectorError();

        std::vector<string> legends2 = {"x"};
        vgp::plot(errors,"Errors",legends2);

        //最適化
        cout << "let's optimize time domain!" << endl;
        //初期値を準備
        Eigen::VectorXd wave_form_coefficients = Eigen::VectorXd::Zero((v2.data().rows()*1.0/Capture->get(CV_CAP_PROP_FPS)+2)*3);
        TimeDomainOptimizer<double> functor4(wave_form_coefficients.rows(),v2.data().rows(),v2);

        NumericalDiff<TimeDomainOptimizer<double>> numDiff4(functor4);
        LevenbergMarquardt<NumericalDiff<TimeDomainOptimizer<double>>> lm4(numDiff4);
        cout << "Before:" << wave_form_coefficients.transpose() << endl;
        int info4 = lm4.minimize(wave_form_coefficients);
        cout << "After:" << wave_form_coefficients.transpose() << endl;
        return 0;

        //最適化
        cout << "let's optimize!" << endl;
        //適切な初期値を準備
        Eigen::MatrixXcd clerped_freq_vectors;

        vsp::Angle2CLerpedFrequency(v2.fs,v2.fc,v2.filteredDataDFT(),clerped_freq_vectors);
        VectorXd complex_frequency_coefficients = VectorXd::Zero((int32_t)((double)clerped_freq_vectors.rows() * v2.fc / v2.fs * 2.0));
        vsp::MatrixXcd2VectorXd(clerped_freq_vectors,complex_frequency_coefficients);

        FrequencyDomainOptimizer<double> functor3(complex_frequency_coefficients.rows(),v2.data().rows(),v2);
        NumericalDiff<FrequencyDomainOptimizer<double>> numDiff3(functor3);
        LevenbergMarquardt<NumericalDiff<FrequencyDomainOptimizer<double>>> lm3(numDiff3);
        cout << "Before:" << complex_frequency_coefficients.transpose() << endl;
        int info3 = lm3.minimize(complex_frequency_coefficients);
        cout << "After:" << complex_frequency_coefficients.transpose() << endl;
        //結果を代入
        //直接は代入できないので、まずcler~に代入
        vsp::VectorXd2MatrixXcd(complex_frequency_coefficients,clerped_freq_vectors);
        //元に戻す
        vsp::Frequency2Angle(clerped_freq_vectors,v2.filteredDataDFT());

        //末尾の余白を削除
        Eigen::MatrixXd buf = v2.filteredDataDFT().block(0,0,v2.data().rows(),v2.data().cols());
        v2.filteredDataDFT() = buf;
        //プロット
        vgp::plot(v2.getRollingVectorError(),"Optimized Errors",legends2);
        vgp::plot(v2.filteredDataDFT(),"Optimized Filterd DFT",legends);
        return 0;
    }


    vsp v2(/*angleQuaternion_vsp2,*/
           division_x,
           division_y,
           rollingShutterDuration,
           convCVMat2EigenMat(matInvDistort),
           convCVMat2EigenMat(matIntrinsic),
           imageSize.width,
           imageSize.height,
           (double)zoomRatio,
           angular_velocity_from_csv,
           Tvideo,
           Tav,
           min_position + subframeOffset,
           (int32_t)(Capture->get(CV_CAP_PROP_FRAME_COUNT)),
           199);
    //平滑化
    v2.filteredDataDFT(Capture->get(CV_CAP_PROP_FPS),1.0);//TODO:引数修正。もはやあまり意味がない。
    v2.filteredQuaternion(100,Capture->get(CV_CAP_PROP_FPS),1.0);

    v2.init_opengl(textureSize);

    //一度動画を閉じて、seek可能版に置き換える
    int32_t e=Capture->get(CV_CAP_PROP_FRAME_COUNT);
    delete Capture;
//    seekableVideoCapture sCapture(videoPass,PREFETCH_LENGTH);


    cv::namedWindow("Preview",cv::WINDOW_NORMAL);
    cv::setWindowProperty("Preview",CV_WND_PROP_FULLSCREEN,CV_WINDOW_FULLSCREEN);
    while(v2.ok()){
        Capture = new cv::VideoCapture(videoPass);//動画をオープン
        for(int32_t i=0;i<e;++i){
            cv::Mat simg;
            if(0 != v2.spin_once(i,*Capture,simg)){
                break;
            }

            if(outputStabilizedVideo){
                std::lock_guard<std::mutex> lock(buffer.mtx);
                buffer.images.push_back(cv::Mat());
                buffer.images.back() = simg.clone();
            }


            //Show fps
            auto t4 = std::chrono::system_clock::now();
            static auto t3 = t4;
            // 処理の経過時間
            double elapsedmicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count() ;
            static double fps = 0.0;
            if(elapsedmicroseconds != 0.0){
                fps = 0.03*(1e6/elapsedmicroseconds) +  0.97*fps;
            }
            t3 = t4;
            printf("fps:%4.2f\r",fps);
            fflush(stdout);


        }

        if(Capture != NULL){
            delete Capture;
            Capture = NULL;
        }

    }

    if(Capture != NULL){
        delete Capture;
        Capture = NULL;
    }
    v2.stop_opengl();

    //動画書き出しのマルチスレッド処理の終了処理
    //書き込みが終わっているか確認
    if(outputStabilizedVideo){
        cout << "Waiting for videoWriter." << endl;
        buffer.isWriting = false;
        th1.join();
    }

#if MULTITHREAD_CAPTURE
    {
        cout << "Wainting for videoCapture." << endl;
        th2.join();
    }
#endif
    cv::destroyAllWindows();

    //音声を付加
    //inputVideoPassにスペースがあったらエスケープシーケンスを追加
    std::string::size_type pos = 0;
    std::string sInputVideoPass = videoPass;
    while(pos=sInputVideoPass.find(" ",pos), pos!=std::string::npos){
        sInputVideoPass.insert(pos,"\\");
        pos+=2;
    }

    //#define OUTPUTVIDEO
    //#ifdef OUTPUTVIDEO
    if(outputStabilizedVideo){
        std::cout << "音声を分離" << std::endl;
        std::string command = "ffmpeg -i " + sInputVideoPass +  " -vn -acodec copy output-audio.aac";
        system(command.c_str());
        std::cout << "音声を結合" << std::endl;
        command = "ffmpeg -i " + sInputVideoPass + "_deblured.avi -i output-audio.aac -codec copy " + sInputVideoPass + "_deblured_audio.avi";
        system(command.c_str());

        system("rm output-audio.aac");
        command = "rm " + sInputVideoPass + "_deblured.avi";
        system(command.c_str());
    }
    //#endif
    return 0;
}

