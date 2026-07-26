#include "gspl/studio/application_controller.hpp"

#include <QCommandLineParser>
#include <QDebug>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QTimer>
#include <QVariant>
#include <Qt>

#include <cstdlib>

namespace {

bool isCriticalQmlWarning(const QQmlError& warning) {
    const auto description = warning.description().toCaseFolded();
    return description.contains(QStringLiteral("is not a type"))
        || description.contains(QStringLiteral("is not installed"))
        || description.contains(QStringLiteral("is not defined"))
        || description.contains(QStringLiteral("invalid property"))
        || description.contains(QStringLiteral("cannot assign"))
        || description.contains(QStringLiteral("referenceerror"))
        || description.contains(QStringLiteral("typeerror"));
}

QObject* findRequiredChild(QObject* root, const char* object_name) {
    return root == nullptr ? nullptr : root->findChild<QObject*>(QString::fromUtf8(object_name));
}

} // namespace

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("GSPL Authoring Studio");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("GSPL");

    QCommandLineParser parser;
    parser.setApplicationDescription("GSPL Authoring Studio");
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption safe_mode(QStringLiteral("safe-mode"), QStringLiteral("Start with external integrations disabled."));
    const QCommandLineOption disable_plugins(QStringLiteral("disable-plugins"), QStringLiteral("Do not discover or start plugin workers."));
    const QCommandLineOption disable_providers(QStringLiteral("disable-providers"), QStringLiteral("Do not discover or start provider workers."));
    const QCommandLineOption reset_layout(QStringLiteral("reset-layout"), QStringLiteral("Ignore persisted layout state for this launch."));
    const QCommandLineOption headless_test(QStringLiteral("headless-test"), QStringLiteral("Load the QML shell, process events, and exit with a startup status."));
    parser.addOption(safe_mode);
    parser.addOption(disable_plugins);
    parser.addOption(disable_providers);
    parser.addOption(reset_layout);
    parser.addOption(headless_test);
    parser.process(app);

    gspl::studio::ApplicationController controller({
        .safe_mode = parser.isSet(safe_mode),
        .disable_plugins = parser.isSet(disable_plugins),
        .disable_providers = parser.isSet(disable_providers),
        .reset_layout = parser.isSet(reset_layout)
    });

    QQmlApplicationEngine engine;
    bool object_created = false;
    bool critical_warning = false;

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
        &app,
        [&](QObject* object, const QUrl& url) {
            Q_UNUSED(url);
            object_created = object != nullptr;
        },
        Qt::DirectConnection);

    QObject::connect(&engine, &QQmlEngine::warnings,
        &app,
        [&](const QList<QQmlError>& warnings) {
            for (const auto& warning : warnings) {
                qWarning().noquote() << warning.toString();
                critical_warning = critical_warning || isCriticalQmlWarning(warning);
            }
        },
        Qt::DirectConnection);

    engine.rootContext()->setContextProperty("gsplStudioVersion", app.applicationVersion());
    engine.rootContext()->setContextProperty("applicationController", &controller);
    engine.loadFromModule("GSPL.Studio", "MainWindow");

    auto validateStartupGraph = [&]() -> bool {
        if (!object_created || engine.rootObjects().isEmpty()) {
            qCritical() << "GSPL Studio startup validation failed: no root object.";
            return false;
        }

        QObject* root = engine.rootObjects().constFirst();
        if (root->objectName() != QStringLiteral("GSPLStudioMainWindow")) {
            qCritical() << "GSPL Studio startup validation failed: unexpected root object" << root->objectName();
            return false;
        }

        if (engine.rootContext()->contextProperty("applicationController").value<QObject*>() != &controller) {
            qCritical() << "GSPL Studio startup validation failed: applicationController missing.";
            return false;
        }

        if (controller.commandModel() == nullptr || controller.commandModel()->count() <= 0) {
            qCritical() << "GSPL Studio startup validation failed: commandModel missing or empty.";
            return false;
        }

        const char* required_children[] = {
            "commandPalette",
            "startupWizard",
            "preferencesDialog",
            "themeSettings",
            "pluginPanel"
        };
        for (const char* child : required_children) {
            if (findRequiredChild(root, child) == nullptr) {
                qCritical() << "GSPL Studio startup validation failed: required component missing" << child;
                return false;
            }
        }

        if (critical_warning) {
            qCritical() << "GSPL Studio startup validation failed: critical QML warnings were emitted.";
            return false;
        }

        qInfo() << "GSPL Studio headless validation passed.";
        return true;
    };

    if (!parser.isSet(headless_test) && engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    if (parser.isSet(headless_test)) {
        QTimer::singleShot(250, &app, [&]() {
            app.exit(validateStartupGraph() ? EXIT_SUCCESS : EXIT_FAILURE);
        });
        QTimer::singleShot(5000, &app, [&]() {
            qCritical() << "GSPL Studio startup validation timed out.";
            app.exit(EXIT_FAILURE);
        });
        return app.exec();
    }

    return app.exec();
}
