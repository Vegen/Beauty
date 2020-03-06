#include <jni.h>
#include <string>
#include <opencv2/opencv.hpp>
#include <android/bitmap.h>
#include "cv_utils.h"

using namespace cv;
using namespace std;

extern "C" {
JNIEXPORT jint JNICALL
Java_com_vegen_beauty_BeautyActivity_beauty(JNIEnv *env, jobject instance, jobject bitmap);

JNIEXPORT void fatsBilateralBlur(Mat &src, Mat &dst, int size, int sigma);

JNIEXPORT int getBlockSum(Mat &sum_mat, int x0, int y0, int x1, int y1, int ch);

JNIEXPORT float getBlockSqSum(Mat &sqsum_mat, int x0, int y0, int x1, int y1, int ch);

JNIEXPORT void skinDetect(const Mat &mat, Mat &mask);

JNIEXPORT void fuseSkin(const Mat &src, const Mat &blur_mat, Mat &dst, const Mat &mask);
}

jint JNICALL
Java_com_vegen_beauty_BeautyActivity_beauty(JNIEnv *env, jobject instance, jobject bitmap) {
    Mat origin;
    cv_utils::bitmap2Mat(env, origin, bitmap);
    if (!origin.data) {
        return -1;
    }

    Mat src;
    // 后续算法在 RBG 基础上，此处转换一下
    cvtColor(origin, src, CV_RGBA2RGB);

    Mat dst;
    int size = 15;
    fatsBilateralBlur(src, dst, size, size * size);
    //cv_utils::mat2Bitmap(env, dst, bitmap);
    Mat skinMask;
    skinDetect(src, skinMask);
    //cv_utils::mat2Bitmap(env, skinMask, bitmap);
    Mat fuseDst;
    fuseSkin(src, dst, fuseDst, skinMask);
    //cv_utils::mat2Bitmap(env, fuseDst, bitmap);

    // 边缘提升
    /*Mat cannyMask;
    Canny(src, cannyMask, 150, 300, 3, false);

    bitwise_and(src, src, fuseDst, cannyMask);*/

    add(fuseDst, Scalar(10, 10, 10), fuseDst);

    cv_utils::mat2Bitmap(env, fuseDst, bitmap);
    return 0;
}

JNIEXPORT // 皮肤区域融合
void fuseSkin(const Mat &src, const  Mat &blur_mat, Mat &dst, const Mat &mask){
    // 融合？
    dst.create(src.size(),src.type());
    GaussianBlur(mask, mask, Size(3, 3), 0.0);
    Mat mask_f;
    mask.convertTo(mask_f, CV_32F);
    normalize(mask_f, mask_f, 1.0, 0.0, NORM_MINMAX);

    int rows = src.rows;
    int cols = src.cols;
    int ch = src.channels();

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < cols; col++)
        {
            // mask_f (1-k)
            /*
            uchar mask_pixels = mask.at<uchar>(row,col);
            // 人脸位置
            if (mask_pixels == 255){
                dst.at<Vec3b>(row, col) = blur_mat.at<Vec3b>(row, col);
            }
            else{
                dst.at<Vec3b>(row, col) = src.at<Vec3b>(row, col);
            }
            */

            // src ，通过指针去获取， 指针 -> Vec3b -> 获取
            uchar b1 = src.at<Vec3b>(row, col)[0];
            uchar g1 = src.at<Vec3b>(row, col)[1];
            uchar r1 = src.at<Vec3b>(row, col)[2];

            // blur_mat
            uchar b2 = blur_mat.at<Vec3b>(row, col)[0];
            uchar g2 = blur_mat.at<Vec3b>(row, col)[1];
            uchar r2 = blur_mat.at<Vec3b>(row, col)[2];

            // dst 254  1
            float k = mask_f.at<float>(row,col);

            dst.at<Vec3b>(row, col)[0] = b2*k + (1 - k)*b1;
            dst.at<Vec3b>(row, col)[1] = g2*k + (1 - k)*g1;
            dst.at<Vec3b>(row, col)[2] = r2*k + (1 - k)*r1;
        }
    }
}

JNIEXPORT void skinDetect(const Mat &src, Mat &skinMask){
    skinMask.create(src.size(), CV_8UC4);
    int rows = src.rows;
    int cols = src.cols;

    Mat ycrcb;
    // COLOR_BGR2YCrCb
    cvtColor(src, ycrcb, COLOR_BGR2YCrCb);

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < cols; col++)
        {
            Vec3b pixels = ycrcb.at<Vec3b>(row, col);
            uchar y = pixels[0];
            uchar cr = pixels[1];
            uchar cb = pixels[2];

            if (y>80 && 85<cb<135 && 135<cr<180){
                skinMask.at<uchar>(row, col) = 255;
            }
            else{
                skinMask.at<uchar>(row, col) = 0;
            }
        }
    }
}


// 积分图的模糊算法 size 模糊的直径
JNIEXPORT void fatsBilateralBlur(Mat &src, Mat &dst, int size, int sigma) {
    // size % 2 == 1
    // 把原来进行填充，方便运算
    Mat mat;
    int radius = size / 2;
    copyMakeBorder(src, mat, radius, radius, radius, radius, BORDER_DEFAULT);
    // 求积分图 (作业去手写积分图的源码)
    Mat sum_mat, sqsum_mat;
    integral(mat, sum_mat, sqsum_mat, CV_32S, CV_32F);

    dst.create(src.size(), src.type());
    int imageH = src.rows;
    int imageW = src.cols;
    int area = size * size;
    // 求四个点，左上，左下，右上，右下
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    int lt = 0, lb = 0, rt = 0, rb = 0;
    int channels = src.channels();
    for (int row = 0; row < imageH; row++) {
        // 思考，x0，y0 , x1 , y1  sum_mat
        // 思考，row, col, dst
        y0 = row;
        y1 = y0 + size;
        for (int col = 0; col < imageW; col++) {
            x0 = col;
            x1 = x0 + size;
            for (int i = 0; i < channels; i++) {
                int sum = getBlockSum(sum_mat, x0, y0, x1, y1, i);
                float sqsum = getBlockSqSum(sqsum_mat, x0, y0, x1, y1, i);

                float diff_sq = (sqsum - (sum * sum) / area) / area;
                float k = diff_sq / (diff_sq + sigma);

                int pixels = src.at<Vec3b>(row, col)[i];
                pixels = (1 - k) * (sum / area) + k * pixels;

                dst.at<Vec3b>(row, col)[i] = pixels;
            }
        }
    }
}

JNIEXPORT int getBlockSum(Mat &sum_mat, int x0, int y0, int x1, int y1, int ch) {
    // 获取四个点的值
    int lt = sum_mat.at<Vec3i>(y0, x0)[ch];
    int lb = sum_mat.at<Vec3i>(y1, x0)[ch];
    int rt = sum_mat.at<Vec3i>(y0, x1)[ch];
    int rb = sum_mat.at<Vec3i>(y1, x1)[ch];

    // 区块的合
    int sum = rb - rt - lb + lt;
    return sum;
}

JNIEXPORT float getBlockSqSum(Mat &sqsum_mat, int x0, int y0, int x1, int y1, int ch) {
    // 获取四个点的值
    float lt = sqsum_mat.at<Vec3f>(y0, x0)[ch];
    float lb = sqsum_mat.at<Vec3f>(y1, x0)[ch];
    float rt = sqsum_mat.at<Vec3f>(y0, x1)[ch];
    float rb = sqsum_mat.at<Vec3f>(y1, x1)[ch];

    // 区块的合
    float sqsum = rb - rt - lb + lt;
    return sqsum;
}
