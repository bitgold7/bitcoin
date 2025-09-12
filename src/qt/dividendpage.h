#ifndef BITCOIN_QT_DIVIDENDPAGE_H
#define BITCOIN_QT_DIVIDENDPAGE_H

#include <QWidget>

class ClientModel;
class WalletModel;
class QLabel;
class PlatformStyle;

class DividendPage : public QWidget
{
    Q_OBJECT
public:
    explicit DividendPage(const PlatformStyle* platformStyle, QWidget* parent = nullptr);
    void setClientModel(ClientModel* model);
    void setWalletModel(WalletModel* model);

public Q_SLOTS:
    void updateData();

private:
    ClientModel* clientModel{nullptr};
    WalletModel* walletModel{nullptr};
    QLabel* poolLabel;
    QLabel* nextLabel;
    const PlatformStyle* m_platform_style;
};

#endif // BITCOIN_QT_DIVIDENDPAGE_H
