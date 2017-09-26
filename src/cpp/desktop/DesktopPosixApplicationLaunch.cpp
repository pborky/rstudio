/*
 * DesktopPosixApplicationLaunch.cpp
 *
 * Copyright (C) 2009-17 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include "DesktopApplicationLaunch.hpp"
#include "DesktopPosixApplication.hpp"
#include "DesktopOptions.hpp"

#include <core/system/Environment.hpp>
#include <core/r_util/RUserData.hpp>

#include <boost/foreach.hpp>

#include <QProcess>
#include <QDebug>

namespace rstudio {
namespace desktop {

namespace {


PosixApplication* app()
{
   return qobject_cast<PosixApplication*>(qApp);
}

} // anonymous namespace

ApplicationLaunch::ApplicationLaunch() :
    QWidget(NULL),
    pMainWindow_(NULL)
{
   connect(app(), SIGNAL(messageReceived(QString)),
           this, SIGNAL(openFileRequest(QString)));

   launchEnv_ = QProcessEnvironment::systemEnvironment();
}

void ApplicationLaunch::init(QString appName,
                             int& argc,
                             char* argv[],
                             boost::scoped_ptr<QApplication>* ppApp,
                             boost::scoped_ptr<ApplicationLaunch>* ppAppLaunch)
{
   // Immediately stuffed into scoped_ptr
   PosixApplication* pSingleApplication = new PosixApplication(appName,
                                                               argc,
                                                               argv);
   // create app launch instance
   pSingleApplication->setApplicationName(appName);
   ppApp->reset(pSingleApplication);

   ppAppLaunch->reset(new ApplicationLaunch());
   pSingleApplication->setAppLauncher(ppAppLaunch->get());

   // connect app open file signal to app launch
   connect(app(), SIGNAL(openFileRequest(QString)),
           ppAppLaunch->get(), SIGNAL(openFileRequest(QString)));
}

void ApplicationLaunch::setActivationWindow(QWidget* pWindow)
{
   pMainWindow_ = pWindow;
   app()->setActivationWindow(pWindow, true);
}


void ApplicationLaunch::activateWindow()
{
   app()->activateWindow();
}

void ApplicationLaunch::attemptToRegisterPeer()
{
   // side-effect of is running is to try to register ourselves as a peer
   app()->isRunning();
}


bool ApplicationLaunch::sendMessage(QString filename)
{
   return app()->sendMessage(filename);
}

QString ApplicationLaunch::startupOpenFileRequest() const
{
   return app()->startupOpenFileRequest();
}

void ApplicationLaunch::launchRStudio(const std::vector<std::string>& args,
                                      const std::string& initialDir)
{
   QStringList argList;
   BOOST_FOREACH(const std::string& arg, args)
   {
      argList.append(QString::fromStdString(arg));
   }

   QString exePath = QString::fromUtf8(
      desktop::options().executablePath().absolutePath().c_str());

   // temporarily restore the library path to the one we were launched with
   std::string ldPath = core::system::getenv("LD_LIBRARY_PATH");
   core::system::setenv("LD_LIBRARY_PATH",
      launchEnv_.value(QString::fromUtf8("LD_LIBRARY_PATH")).toStdString());

   // set environment variable indicating initial launch dir
   core::system::setenv(kRStudioInitialWorkingDir, initialDir);

   // start RStudio detached
   QProcess::startDetached(exePath, argList);

   // restore environment
   core::system::setenv("LD_LIBRARY_PATH", ldPath);
   core::system::unsetenv(kRStudioInitialWorkingDir);
}

} // namespace desktop
} // namespace rstudio
