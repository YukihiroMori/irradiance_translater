#include "irradiance_translater.h"

using namespace std;
using namespace cv;
using namespace glm;

#define SourceHDR "source.hdr"
#define ResizeHDR "resize.hdr"
#define ResultIrradiance "irradiance.hdr"

#define Resize
#define Gain

#define ResultWidth 256
#define ResultHeight 128

int main(int argc, const char* argv[])
{

#pragma region Declaration
	//ループ内の変数は事前に宣言

	double s;
	double t;
	double theta;
	double phi;

	dvec3 n;

	double ws;
	double wt;
	double wtheta;
	double wphi;

	dvec3 w;

	double nw;

	double d_wtheta;
	double d_wphi;

	double dw;

	dvec3 result;

	vec3 *hdr;
	vec3 *dst;

#pragma endregion

	//入力画像ファイルパス
	string path(SourceHDR);
	
	//入力環境マップ読み込み(正距円筒図法の輝度マップ)
	Mat SRC = imread(path, -1);

	if (SRC.empty()) {
		//入力画像なしの場合は終了
		cout << "File not found" << endl;
		return 0;
	}

#ifdef  Resize
	//サイズ変更有りの場合のみ入力画像をリサイズ
	resize(SRC, SRC, Size(ResultWidth, ResultHeight), 0.0, 0.0, cv::INTER_AREA);
#endif 

	//出力画像宣言(入力画像と同サイズ、同チャンネル)
	Mat DST;
	DST = SRC.clone();

	//輝き係数
	double shi = 1;
	//正規化ファクター(M_PIを除くと純粋照度)
	double gain = (shi + 1) / (2 * M_PI);

	//走査用画像サイズ(入力サイズ=出力サイズ)
	double width = SRC.cols;
	double height = SRC.rows;

	//2π事前計算
	double M_2PI = 2.0f * M_PI;

	cout << "計算開始" << endl;

	//各出力画素(照度)についてループ
	for (int v = 0; v < height; v++) {
		for (int u = 0; u < width; u++) {
			//結果保存用変数　初期化
			result = dvec3(0.0f);

			//u,vを[0~1]に正規化
			s = (u + 0.5f) / width; //0.5fは画素の中心位置をベクトルの向きとして計算する為
			t = (v + 0.5f) / height;

			//sを[0~π],tを[0~2π]に射影
			theta = M_PI * t;
			phi = M_2PI * s;

			//照度計算する面の法線ベクトルを計算
			n = dvec3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));


			/*   
				照度計算
				E = ∫ L(ω)(n.ω)dω
				  = ∬ L(θ,φ)(n.ω)sinθdθdφ

				  緯度[0~height]⇒[0~π]
				  径度[0~width]⇒[0~2π]
				  に射影しているので

				  縦方向の微小面積
				  dθ=π / height
				  横方向の微小面積
				  dφ=2π / width

				  L(θ,φ)は、入力画素
			*/

			//全入力画素についてループ
			for (int wv = 0; wv < height; wv++) {
				for (int wu = 0; wu < width; wu++) {

					//wu,wvを[0~1]に正規化
					ws = (wu + 0.5f) / width;
					wt = (wv + 0.5f) / height;

					//wsを[0~π],wtを[0~2π]に射影
					wtheta = M_PI * wt;
					wphi = M_2PI * ws;

					//入射ベクトル計算
					w = dvec3(sin(wtheta)*cos(wphi), sin(wtheta)*sin(wphi), cos(wtheta));

					//面法線と入射光ベクトルの内積
					nw = dot(n, w);

					//照度は入射光が面方向を向いている時のみ計算 : GPUの場合max(0.0 , nw)
					if (nw > 0.0f) { 

						//微小面積計算
						d_wtheta = M_PI / height;
						d_wphi = M_2PI / width;

						//立体角計算
						dw = sin(wtheta) * d_wtheta * d_wphi;

#ifdef  Gain
						//入射輝度×微小面積により照度計算
						hdr = SRC.ptr<vec3>(wv);
						result += gain * (dvec3)hdr[wu] * pow(nw, shi) * dw;
						//gainは輝き係数によって凹んだ面積をかさ上げする。(面積1に正規化)
#endif //  Gain
#ifndef Gain
						//入射輝度×微小面積により照度計算
						hdr = SRC.ptr<vec3>(wv);
						result += (dvec3)hdr[wu] * pow(nw, shi) * dw;
#endif // !Gain
					}
				}
			}

			//計算結果を照度マップに格納
			dst = DST.ptr<vec3>(v);
			dst[u] = result;
		}
	}

	cout << "計算終了" << endl;

	//結果表示用Window
	namedWindow("Source", CV_WINDOW_AUTOSIZE);
	namedWindow("Result", CV_WINDOW_AUTOSIZE);

	//結果表示
	imshow("Source", SRC);
	imshow("Result", DST);

	//結果保存
	imwrite(ResizeHDR, SRC);
	imwrite(ResultIrradiance, DST);

	//wait
	waitKey();

	//Window破棄
	destroyAllWindows();

	return 0;
}