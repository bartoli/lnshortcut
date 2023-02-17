#ifndef GUITHREAD_HPP
#define GUITHREAD_HPP

#include <QThread>

/*
 * Class for the thrread managing UI stuff.
 */

class GuiThread : public QThread
{
    void run() override;
};

#endif // GUITHREAD_HPP
