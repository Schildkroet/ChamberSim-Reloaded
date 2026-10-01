#pragma once

#include <QDialog>

class QComboBox;
class QSpinBox;

// File > Settings: options that apply to the whole application. Changes take effect with OK or Apply.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    [[nodiscard]] int fontSize() const;

private:
    void apply();

    QSpinBox *m_fontSize;
    QComboBox *m_unitSystem;
    int m_originalSize;
    int m_originalSystem;
};
