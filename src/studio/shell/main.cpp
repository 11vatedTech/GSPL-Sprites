#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "gspl/studio/project.hpp"
#include "gspl/studio/workspace.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("GSPL Authoring Studio");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("GSPL");

    QQmlApplicationEngine engine;

    qmlRegisterType<gspl::studio::Project>("GSPL.Studio", 1, 0, "Project");

    auto workspace = std::make_shared<gspl::studio::Workspace>(
        gspl::studio::Workspace::Config{std::filesystem::current_path() / "workspace"}
    );
    engine.rootContext()->setContextProperty("workspace", workspace.get());

    engine.load("qrc:/studio/shell/MainWindow.qml");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
