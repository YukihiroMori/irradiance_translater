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
	//���[�v���̕ϐ��͎��O�ɐ錾

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

	//���͉摜�t�@�C���p�X
	string path(SourceHDR);
	
	//���͊��}�b�v�ǂݍ���(�����~���}�@�̋P�x�}�b�v)
	Mat SRC = imread(path, -1);

	if (SRC.empty()) {
		//���͉摜�Ȃ��̏ꍇ�͏I��
		cout << "File not found" << endl;
		return 0;
	}

#ifdef  Resize
	//�T�C�Y�ύX�L��̏ꍇ�̂ݓ��͉摜�����T�C�Y
	resize(SRC, SRC, Size(ResultWidth, ResultHeight), 0.0, 0.0, cv::INTER_AREA);
#endif 

	//�o�͉摜�錾(���͉摜�Ɠ��T�C�Y�A���`�����l��)
	Mat DST;
	DST = SRC.clone();

	//�P���W��
	double shi = 1;
	//���K���t�@�N�^�[(M_PI�������Ə����Ɠx)
	double gain = (shi + 1) / (2 * M_PI);

	//�����p�摜�T�C�Y(���̓T�C�Y=�o�̓T�C�Y)
	double width = SRC.cols;
	double height = SRC.rows;

	//2�Ύ��O�v�Z
	double M_2PI = 2.0f * M_PI;

	cout << "�v�Z�J�n" << endl;

	//�e�o�͉�f(�Ɠx)�ɂ��ă��[�v
	for (int v = 0; v < height; v++) {
		for (int u = 0; u < width; u++) {
			//���ʕۑ��p�ϐ��@������
			result = dvec3(0.0f);

			//u,v��[0~1]�ɐ��K��
			s = (u + 0.5f) / width; //0.5f�͉�f�̒��S�ʒu���x�N�g���̌����Ƃ��Čv�Z�����
			t = (v + 0.5f) / height;

			//s��[0~��],t��[0~2��]�Ɏˉe
			theta = M_PI * t;
			phi = M_2PI * s;

			//�Ɠx�v�Z����ʂ̖@���x�N�g�����v�Z
			n = dvec3(sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));


			/*   
				�Ɠx�v�Z
				E = �� L(��)(n.��)d��
				  = �� L(��,��)(n.��)sin��d��d��

				  �ܓx[0~height]��[0~��]
				  �a�x[0~width]��[0~2��]
				  �Ɏˉe���Ă���̂�

				  �c�����̔����ʐ�
				  d��=�� / height
				  �������̔����ʐ�
				  d��=2�� / width

				  L(��,��)�́A���͉�f
			*/

			//�S���͉�f�ɂ��ă��[�v
			for (int wv = 0; wv < height; wv++) {
				for (int wu = 0; wu < width; wu++) {

					//wu,wv��[0~1]�ɐ��K��
					ws = (wu + 0.5f) / width;
					wt = (wv + 0.5f) / height;

					//ws��[0~��],wt��[0~2��]�Ɏˉe
					wtheta = M_PI * wt;
					wphi = M_2PI * ws;

					//���˃x�N�g���v�Z
					w = dvec3(sin(wtheta)*cos(wphi), sin(wtheta)*sin(wphi), cos(wtheta));

					//�ʖ@���Ɠ��ˌ��x�N�g���̓���
					nw = dot(n, w);

					//�Ɠx�͓��ˌ����ʕ����������Ă��鎞�̂݌v�Z : GPU�̏ꍇmax(0.0 , nw)
					if (nw > 0.0f) { 

						//�����ʐόv�Z
						d_wtheta = M_PI / height;
						d_wphi = M_2PI / width;

						//���̊p�v�Z
						dw = sin(wtheta) * d_wtheta * d_wphi;

#ifdef  Gain
						//���ˋP�x�~�����ʐςɂ��Ɠx�v�Z
						hdr = SRC.ptr<vec3>(wv);
						result += gain * (dvec3)hdr[wu] * pow(nw, shi) * dw;
						//gain�͋P���W���ɂ���ĉ��񂾖ʐς������グ����B(�ʐ�1�ɐ��K��)
#endif //  Gain
#ifndef Gain
						//���ˋP�x�~�����ʐςɂ��Ɠx�v�Z
						hdr = SRC.ptr<vec3>(wv);
						result += (dvec3)hdr[wu] * pow(nw, shi) * dw;
#endif // !Gain
					}
				}
			}

			//�v�Z���ʂ��Ɠx�}�b�v�Ɋi�[
			dst = DST.ptr<vec3>(v);
			dst[u] = result;
		}
	}

	cout << "�v�Z�I��" << endl;

	//���ʕ\���pWindow
	namedWindow("Source", CV_WINDOW_AUTOSIZE);
	namedWindow("Result", CV_WINDOW_AUTOSIZE);

	//���ʕ\��
	imshow("Source", SRC);
	imshow("Result", DST);

	//���ʕۑ�
	imwrite(ResizeHDR, SRC);
	imwrite(ResultIrradiance, DST);

	//wait
	waitKey();

	//Window�j��
	destroyAllWindows();

	return 0;
}