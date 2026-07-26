#include "gspl/studio/application_controller.hpp"

#include <QCoreApplication>
#include <cassert>
#include <cstdio>

namespace {

void test_command_model_roles_and_lookup() {
    gspl::studio::CommandModel model;
    model.addCommand({"Open Project", "Ctrl+O", "openProject", true});
    model.addCommand({"Save", "Ctrl+S", "save", false});

    assert(model.count() == 2);
    assert(model.rowCount() == 2);
    assert(model.rowCount(model.index(0, 0)) == 0);

    const auto roles = model.roleNames();
    assert(roles.value(gspl::studio::CommandModel::NameRole) == "name");
    assert(roles.value(gspl::studio::CommandModel::ShortcutRole) == "shortcut");
    assert(roles.value(gspl::studio::CommandModel::CommandIdRole) == "commandId");
    assert(roles.value(gspl::studio::CommandModel::EnabledRole) == "enabled");

    const auto first = model.index(0, 0);
    assert(model.data(first, gspl::studio::CommandModel::NameRole).toString() == "Open Project");
    assert(model.data(first, gspl::studio::CommandModel::ShortcutRole).toString() == "Ctrl+O");
    assert(model.data(first, gspl::studio::CommandModel::CommandIdRole).toString() == "openProject");
    assert(model.data(first, gspl::studio::CommandModel::EnabledRole).toBool());
    assert(model.commandIdAt(0) == "openProject");
    assert(model.commandIdAt(99).isEmpty());
    assert(model.data(QModelIndex{}, gspl::studio::CommandModel::NameRole).isNull());
}

void test_command_model_filtering_and_enabled_state() {
    gspl::studio::CommandModel model;
    model.addCommand({"Open Project", "Ctrl+O", "openProject", true});
    model.addCommand({"Save", "Ctrl+S", "save", false});
    model.addCommand({"Save All", "Ctrl+Shift+S", "saveAll", false});

    bool filter_changed = false;
    QObject::connect(&model, &gspl::studio::CommandModel::filterChanged, [&]() {
        filter_changed = true;
    });

    model.setFilter("save");
    assert(filter_changed);
    assert(model.count() == 2);
    assert(model.commandIdAt(0) == "save");
    assert(!model.isCommandEnabled("save"));

    model.setCommandEnabled("save", true);
    assert(model.isCommandEnabled("save"));
}

void test_application_controller_options_and_notifications() {
    gspl::studio::ApplicationController controller({
        .safe_mode = true,
        .disable_plugins = true,
        .disable_providers = true,
        .reset_layout = true
    });

    assert(controller.safeMode());
    assert(controller.pluginsDisabled());
    assert(controller.providersDisabled());
    assert(controller.resetLayout());
    assert(controller.commandModel() != nullptr);
    assert(controller.commandModel()->count() > 0);

    bool status_changed = false;
    QObject::connect(&controller, &gspl::studio::ApplicationController::statusMessageChanged, [&]() {
        status_changed = true;
    });

    controller.dispatchCommand("preferences");
    assert(status_changed);
    assert(controller.statusMessage().contains("preferences"));
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    test_command_model_roles_and_lookup();
    test_command_model_filtering_and_enabled_state();
    test_application_controller_options_and_notifications();
    std::printf("ApplicationController Qt tests passed.\n");
    return 0;
}
