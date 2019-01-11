#include "demo.h"
#include <QtMath>


Demo::Demo(QObject *parent) : QObject(parent),
    enc1Val(0), enc2Val(0), distVal(50),
    demoModePeriod(this), demoModeState(idle),
    demoCycleCount(0)
{
    //demoModePeriod.setInterval(50);

    connect(&demoModePeriod, SIGNAL(timeout()),
            this, SLOT(handleDemoModePeriod()));

    resetDemoModeTimer();
}

void Demo::resetDemoModeTimer()
{
    demoCycleCount = 0;
    demoModeCurStepInd = 1;
    demoModeState = idle;
    demoModePeriod.setSingleShot(false);
    demoModePeriod.setInterval(60*1000);
    demoModePeriod.start();
}

void Demo::startDemo()
{
    demoModePeriod.setInterval(50);
    demoModeCurStepInd = 1;
    demoModeState = idle;
}

void Demo::stopDemo()
{
    resetDemoModeTimer();

}

void Demo::enableDemo(bool bEna)
{
    resetDemoModeTimer();
    if(bEna ==  true){
        startDemo();
        emit msg("start demo mode");
    }
    else{
        demoModePeriod.stop();
        emit msg("stop demo mode");
    }
}

void Demo::handleDemoModePeriod()
{
    int intervalsInSecond = 1000/demoModePeriod.interval();
    qreal sinVal;
    switch(demoModeState){
    case idle:
        //emit msg("human interface timeout. Start demo mode");
        demoModePeriod.setInterval(50);
        demoModeState = idleTimeout;
        demoCycleCount++;
        break;
    case idleTimeout:
        demoModeCurStepInd--;
        if(demoModeCurStepInd <= 0){
            demoModeState = walkIn;
        }
        break;
    case walkIn:
        //emit msg("human interface timeout. Start demo mode. walkIn");
        if(distVal <= 5){
            demoModeState = moving;
            //enc1ValMovingStep = randGen.bounded(-50, 50);
            //enc2ValMovingStep = randGen.bounded(-50, 50);
            //qDebug("movingStep %d %d", enc1ValMovingStep, enc2ValMovingStep);

            //enc1ValMovingOffset = random_int(-4000, 4000); //randGen.bounded(-4000, 4000);
            enc2ValMovingOffset = randGen.bounded(-4000, 4000);
            enc1EvolStart = enc1Val;
            enc2EvolStart = enc2Val;
            //demoModeSteps = random_int(2, 6)*intervalsInSecond;
            demoModeSteps = randGen.bounded(2, 6)*intervalsInSecond;
            demoModeCurStepInd = demoModeSteps;
            qDebug("ints:%d movingOffset:%d %d %d %d", demoModeSteps/intervalsInSecond, enc1ValMovingOffset, enc2ValMovingOffset, enc1Val, enc2Val);
            emit msg(QString("ints:%1 movingOffset:%2 %3 %4 %5").arg(demoModeSteps/intervalsInSecond).arg(enc1ValMovingOffset).arg(enc2ValMovingOffset).arg(enc1Val).arg(enc2Val));
        }
        else{
            distVal --;
        }
        break;
    case moving:

        //emit msg("human interface timeout. Start demo mode. moveLeft");
        demoModeCurStepInd--;
        sinVal = (qSin(-M_PI_2 + M_PI*(demoModeSteps-demoModeCurStepInd)/(float)demoModeSteps)+1)/2;
        enc1Val = (enc1EvolStart+(int)(enc1ValMovingOffset*sinVal))&0x1fff;
        enc2Val = (enc2EvolStart+(int)(enc2ValMovingOffset*sinVal))&0x1fff;; //(enc2Val-1)&0x1fff;

        qDebug("%f %d %d", sinVal, enc1Val, enc2Val);
        emit msg(QString("encVal: %1 %2").arg(enc1Val).arg(enc2Val));

        if(demoModeCurStepInd <= 0){
            //demoModeCurStepInd = random_int(5*intervalsInSecond, 20*intervalsInSecond); //randGen.bounded(5*intervalsInSecond, 20*intervalsInSecond); //125;
            demoModeCurStepInd = randGen.bounded(5*intervalsInSecond, 20*intervalsInSecond);
            demoModeState = hold;
            qDebug("hold for %d", demoModeCurStepInd/intervalsInSecond);
            emit msg(QString("hold for %1").arg(demoModeCurStepInd/intervalsInSecond));
        }
        break;
    case hold:
        //emit msg("human interface timeout. Start demo mode. moveRight");

        //enc1Val = (enc1Val+enc1Offset+enc1ValMovingStep)&0x1fff;
        //enc2Val = (enc2Val+enc2Offset+enc2ValMovingStep)&0x1fff; //(enc2Val-1)&0x1fff;

        demoModeCurStepInd--;
        if(demoModeCurStepInd <= 0){
            //bool bEndIter = randomBool();
            bool bEndIter = (bool)randGen.bounded(2);
            if(bEndIter == true){
                demoModeState = walkOut;
                qDebug("walkOut");
                emit msg(QString("walkOut"));
            }
            else{
                demoModeState = walkIn;
            }
        }
        break;
    case walkOut:
        //emit msg("human interface timeout. Start demo mode. walkOut");
        distVal ++;
        if(distVal >= 50){
            //demoModeSteps = random_int(10, 20)*intervalsInSecond; //
            demoModeSteps = randGen.bounded(10, 20)*intervalsInSecond;
            demoModeCurStepInd = demoModeSteps;
            demoModeState = idle;
            qDebug("idle for %d", demoModeCurStepInd/intervalsInSecond);
            emit msg(QString("idle for %1").arg(demoModeCurStepInd/intervalsInSecond));
        }
        break;

    }
    emit newPosData(enc1Val, enc2Val, distVal);
}
