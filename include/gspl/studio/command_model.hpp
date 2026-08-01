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

} // namespace gspl::studio
