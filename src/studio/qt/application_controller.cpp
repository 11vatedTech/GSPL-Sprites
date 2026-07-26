#include "gspl/studio/application_controller.hpp"

#include <algorithm>
#include <utility>

namespace gspl::studio {

CommandModel::CommandModel(QObject* parent)
    : QAbstractListModel(parent) {}

int CommandModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(visible_rows_.size());
}

QVariant CommandModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= visible_rows_.size()) {
        return {};
    }

    const auto& command = commands_.at(visible_rows_.at(index.row()));
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return command.name;
    case ShortcutRole:
        return command.shortcut;
    case CommandIdRole:
        return command.command_id;
    case EnabledRole:
        return command.enabled;
    default:
        return {};
    }
}

QHash<int, QByteArray> CommandModel::roleNames() const {
    return {
        {NameRole, "name"},
        {ShortcutRole, "shortcut"},
        {CommandIdRole, "commandId"},
        {EnabledRole, "enabled"}
    };
}

QString CommandModel::filter() const {
    return filter_;
}

void CommandModel::setFilter(const QString& filter) {
    if (filter_ == filter) {
        return;
    }
    filter_ = filter;
    rebuildVisibleRows();
    emit filterChanged();
}

int CommandModel::count() const {
    return static_cast<int>(visible_rows_.size());
}

QString CommandModel::commandIdAt(int row) const {
    if (row < 0 || row >= visible_rows_.size()) {
        return {};
    }
    return commands_.at(visible_rows_.at(row)).command_id;
}

bool CommandModel::isCommandEnabled(const QString& command_id) const {
    const auto it = std::find_if(commands_.cbegin(), commands_.cend(), [&](const Command& command) {
        return command.command_id == command_id;
    });
    return it != commands_.cend() && it->enabled;
}

void CommandModel::addCommand(Command command) {
    commands_.push_back(std::move(command));
    rebuildVisibleRows();
}

void CommandModel::setCommandEnabled(const QString& command_id, bool enabled) {
    for (auto& command : commands_) {
        if (command.command_id == command_id && command.enabled != enabled) {
            command.enabled = enabled;
            rebuildVisibleRows();
            return;
        }
    }
}

void CommandModel::rebuildVisibleRows() {
    beginResetModel();
    visible_rows_.clear();
    const auto needle = filter_.trimmed().toCaseFolded();
    for (int i = 0; i < commands_.size(); ++i) {
        const auto haystack = commands_.at(i).name.toCaseFolded();
        if (needle.isEmpty() || haystack.contains(needle)) {
            visible_rows_.push_back(i);
        }
    }
    endResetModel();
}

ApplicationController::ApplicationController(StartupOptions options, QObject* parent)
    : QObject(parent), options_(options) {
    registerDefaultCommands();
    if (options_.safe_mode) {
        setStatusMessage("Safe mode active");
    }
}

CommandModel* ApplicationController::commandModel() {
    return &command_model_;
}

QString ApplicationController::statusMessage() const {
    return status_message_;
}

bool ApplicationController::safeMode() const {
    return options_.safe_mode;
}

bool ApplicationController::pluginsDisabled() const {
    return options_.disable_plugins;
}

bool ApplicationController::providersDisabled() const {
    return options_.disable_providers;
}

bool ApplicationController::resetLayout() const {
    return options_.reset_layout;
}

void ApplicationController::dispatchCommand(const QString& command_id) {
    if (!command_model_.isCommandEnabled(command_id)) {
        setStatusMessage(QStringLiteral("Command unavailable: %1").arg(command_id));
        return;
    }
    emit commandDispatched(command_id);
    setStatusMessage(QStringLiteral("Command dispatched: %1").arg(command_id));
}

void ApplicationController::registerDefaultCommands() {
    command_model_.addCommand({"New Project", "Ctrl+N", "newProject", true});
    command_model_.addCommand({"Open Project", "Ctrl+O", "openProject", true});
    command_model_.addCommand({"Close Project", "Ctrl+W", "closeProject", false});
    command_model_.addCommand({"Save", "Ctrl+S", "save", false});
    command_model_.addCommand({"Save All", "Ctrl+Shift+S", "saveAll", false});
    command_model_.addCommand({"Undo", "Ctrl+Z", "undo", false});
    command_model_.addCommand({"Redo", "Ctrl+Shift+Z", "redo", false});
    command_model_.addCommand({"Cut", "Ctrl+X", "cut", true});
    command_model_.addCommand({"Copy", "Ctrl+C", "copy", true});
    command_model_.addCommand({"Paste", "Ctrl+V", "paste", true});
    command_model_.addCommand({"Build", "Ctrl+B", "build", false});
    command_model_.addCommand({"Rebuild", "Ctrl+Shift+B", "rebuild", false});
    command_model_.addCommand({"Clean", "", "clean", false});
    command_model_.addCommand({"Preferences", "Ctrl+,", "preferences", true});
    command_model_.addCommand({"About GSPL Studio", "", "about", true});
}

void ApplicationController::setStatusMessage(QString message) {
    if (status_message_ == message) {
        return;
    }
    status_message_ = std::move(message);
    emit statusMessageChanged();
}

} // namespace gspl::studio
