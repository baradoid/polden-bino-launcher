#pragma once

#include <QObject>
#include <QRandomGenerator>
#include <QTimer>

class Demo : public QObject
{
    Q_OBJECT
public:
    explicit Demo(QObject *parent = nullptr);
    void startDemo();
    void stopDemo();
    void enableDemo(bool);

    uint32_t demoCycleCount;

    enum TDemoModeState {
        idle, idleTimeout, walkIn, moving, hold, walkOut
    };

    Q_ENUM(TDemoModeState)

    int demoModeSteps, demoModeCurStepInd;
    QTimer demoModePeriod;
    TDemoModeState demoModeState;

    QRandomGenerator randGen;

private:
    uint16_t enc1Val, enc2Val, distVal;
    //int16_t enc1ValMovingStep, enc2ValMovingStep;
    int16_t enc1ValMovingOffset, enc2ValMovingOffset;
    int16_t enc1EvolStart, enc2EvolStart;
    void resetDemoModeTimer();

    //    bool randomBool()
    //    {
    //        return 0 + (qrand() % (1 - 0 + 1)) == 1;
    //    }

    //    int random_int(int min, int max)
    //    {
    //       return min + qrand() % (max+1 - min);
    //    }

signals:
    void newPosData(uint16_t enc1Val, uint16_t enc2Val, int dist);
    void msg(QString);

private slots:
    void handleDemoModePeriod();
};

