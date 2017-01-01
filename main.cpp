#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/objdetect.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include <time.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>

using namespace cv;
using namespace std;

String face_cascade_name = "haarcascade_frontalface_alt.xml";
CascadeClassifier face_cascade;

int player_x = 200;
int wide = 480;

Mat detectAndMask( Mat frame );
void overlayImage(const Mat &background, const Mat &foreground, Mat &output, Point2i location);

int main(int argc, char *argv[])
{
    Mat bg = imread("bg.png", IMREAD_UNCHANGED);
    Mat met = imread("meteor.png", IMREAD_UNCHANGED);
    Mat ship = imread("ship.jpg", IMREAD_UNCHANGED);

    namedWindow( "Scene", CV_WINDOW_AUTOSIZE );

    if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading face cascade\n"); return -1; };

    VideoCapture capture(0);
    Mat frame;
    if ( ! capture.isOpened() ) { printf("--(!)Error opening video capture\n"); return -1; }
    int bgy = bg.rows;
    int mety = -108;
    int metx = 30;
    bool has_met = false;
    int score = 0;
    srand(time(NULL));

    while ( capture.read(frame) ) {
        if( frame.empty() ) {
            printf(" --(!) No captured frame -- Break!");
            break;
        }

        Mat player = detectAndMask( frame );

        Mat scene = bg(Rect(0,bgy-720-1,frame.cols,720)); // x,y,width,height
        Mat new_scene;
        int player_x_screen = player_x * frame.cols / wide - player.cols/2;
        overlayImage(scene, player, new_scene,Point(player_x_screen, 740 - player.rows));

        if(!has_met){
            //srand(time(NULL));
            //metx = (rand()%3 * 230 + 30); // 3-way
            metx = (rand()%(frame.cols-120)); // random
            overlayImage(new_scene, met, new_scene,Point(metx,-108));
            has_met = true;
        } else {
            if(mety >= 400){
                if(abs((player_x_screen + player.cols/2) - (metx + 60) ) < 110){
                    return score;
                }
            }
            if(mety >= 720){
                mety = -120;
                has_met = false;
                score++;
            } else {
                overlayImage(new_scene, met, new_scene,Point(metx,mety));
            }
        }

        string text = "Score: " + to_string(score);
        putText(new_scene, text, Point(20,40), FONT_HERSHEY_DUPLEX , 1, Scalar(0,0,255));

        imshow("Scene", new_scene);
        waitKey(40);
        bgy -= 16;
        if(bgy < 1280){
            bgy = bg.rows;
        }
        mety += 12 + score*2;
    }

    waitKey(0);
    return 0;
}

Mat detectAndMask( Mat frame )
{
    std::vector<Rect> faces;
    Mat frame_gray;
    Mat ship = imread("ship.png",IMREAD_UNCHANGED);

    cvtColor( frame, frame_gray, COLOR_BGR2GRAY );

    equalizeHist( frame_gray, frame_gray );

    face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CASCADE_SCALE_IMAGE, Size(80, 80) );

    if(faces.size() == 0){
        return ship;
    }

    Mat faceROI = frame( faces[0] );
    player_x = frame.cols - faces[0].x - faces[0].width;
    wide = frame.cols - faces[0].width;

    Mat mask = imread("mask.jpg",IMREAD_GRAYSCALE);

    resize(mask,mask,Size(faceROI.cols,faceROI.rows));

    vector<Mat> channels;
    split(faceROI, channels);

    channels.push_back(mask);
    Mat result;
    merge(channels, result);
    resize(result,result,Size(100,100));

    Mat player;
    overlayImage(ship, result, player,Point(26,68));

    return player;

}

void overlayImage(const Mat &background, const Mat &foreground, Mat &output, Point2i location){
    background.copyTo(output);

    // start at the row indicated by location, or at row 0 if location.y is negative.
    for(int y = std::max(location.y , 0); y < background.rows; ++y) {
        int fY = y - location.y; // because of the translation

        // we are done of we have processed all rows of the foreground image.
        if(fY >= foreground.rows)
            break;

        // start at the column indicated by location,
        // or at column 0 if location.x is negative.
        for(int x = std::max(location.x, 0); x < background.cols; ++x) {
            int fX = x - location.x; // because of the translation.

            // we are done with this row if the column is outside of the foreground image.
            if(fX >= foreground.cols)
                break;

            // determine the opacity of the foregrond pixel, using its fourth (alpha) channel.
            double opacity = ((double)foreground.data[fY * foreground.step + fX * foreground.channels() + 3]) / 255.;

            // and now combine the background and foreground pixel, using the opacity,
            // but only if opacity > 0.
            for(int c = 0; opacity > 0 && c < output.channels(); ++c) {
                unsigned char foregroundPx = foreground.data[fY * foreground.step + fX * foreground.channels() + c];
                unsigned char backgroundPx = background.data[y * background.step + x * background.channels() + c];
                output.data[y*output.step + output.channels()*x + c] = backgroundPx * (1.-opacity) + foregroundPx * opacity;
            }
        }
    }
}


