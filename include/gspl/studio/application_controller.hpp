#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVector>

namespace gspl::studio {

class CommandModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        ShortcutRole,
        CommandIdRole,
        EnabledRole
    };

    struct Command {
        QString name;
        QString shortcut;
        QString command_id;
        bool enabled{true};
    };

    explicit CommandModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString filter() const;
    void setFilter(const QString& filter);

    Q_INVOKABLE int count() const;
    Q_INVOKABLE QString commandIdAt(int row) const;
    Q_INVOKABLE bool isCommandEnabled(const QString& command_id) const;

    void addCommand(Command command);
    void setCommandEnabled(const QString& command_id, bool enabled);

signals:
    void filterChanged();

private:
    void rebuildVisibleRows();

    QVector<Command> commands_;
    QVector<int> visible_rows_;
    QString filter_;
};

class ApplicationController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(CommandModel* commandModel READ commandModel CONSTANT)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool safeMode READ safeMode CONSTANT)
    Q_PROPERTY(bool pluginsDisabled READ pluginsDisabled CONSTANT)
    Q_PROPERTY(bool providersDisabled READ providersDisabled CONSTANT)
    Q_PROPERTY(bool resetLayout READ resetLayout CONSTANT)

public:
    struct StartupOptions {
        bool safe_mode{false};
        bool disable_plugins{false};
        bool disable_providers{false};
        bool reset_layout{false};
    };

    explicit ApplicationController(StartupOptions options, QObject* parent = nullptr);

    [[nodiscard]] CommandModel* commandModel();
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] bool safeMode() const;
    [[nodiscard]] bool pluginsDisabled() const;
    [[nodiscard]] bool providersDisabled() const;
    [[nodiscard]] bool resetLayout() const;

    Q_INVOKABLE void dispatchCommand(const QString& command_id);

signals:
    void commandDispatched(QString command_id);
    void statusMessageChanged();

private:
    void registerDefaultCommands();
    void setStatusMessage(QString message);

    StartupOptions options_;
    CommandModel command_model_;
    QString status_message_{"Ready"};
};

} // namespace gspl::studio
