#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "computation.h"
#include <QString>
#include <QRegExp>
#include <Q3DScatter>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    chart = new Q3DScatter;
    QWidget *container = QWidget::createWindowContainer(chart);
    ui->vis_chart->addWidget(container, 1);
    chart->setAspectRatio(1.0);
    chart->setHorizontalAspectRatio(1.0);
    chart->setShadowQuality(QAbstract3DGraph::ShadowQualityNone); // отключаем тени
    chart->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_start_clicked()
{
    start(true);
}

void MainWindow::start(bool from_gui_inputs){
    target_x = ui->edt_target_x->text().toFloat();
    target_y = ui->edt_target_y->text().toFloat();

    // Два способа ввода начальных данных: через пользовательский ввод и через специальную строку параметров (больше точность)
    if (from_gui_inputs && !ui->btn_optres->isChecked()){
        v0 = ui->edt_v0->text().toFloat();
        alpha = deg_to_rad(get_alpha());
        beta = deg_to_rad(get_beta());
        u_value = ui->edt_u->text().toFloat();
        gamma = deg_to_rad(get_gamma());
        mu = ui->edt_mu->text().toFloat();
        m = ui->edt_m->text().toFloat();
        dt = ui->edt_dt->text().toFloat();
    } else {
        QStringList params_list = ui->edt_optres->text().split(";");
        v0 = params_list[0].toFloat();
        alpha = deg_to_rad(params_list[1].toFloat());
        beta = deg_to_rad(params_list[2].toFloat());
        u_value = params_list[3].toFloat();
        gamma = deg_to_rad(params_list[4].toFloat());
        mu = params_list[5].toFloat();
        m = params_list[6].toFloat();
        dt = params_list[7].toFloat();
    }

    Vec v = Vec(v0, alpha, beta);
    Vec u = Vec(u_value, 0.0, gamma);

    Simulation result = compute(v, u, mu, m, dt);
    P end = P(result.data.back().x(), result.data.back().z(), 0.0);
    AVec v_end = result.v_end;
    float alpha_end = asin(abs(v_end.z/v_end.length()));

    ui->label_end->setText("(" + QString::number(end.x, 'f', 1) + ", " + QString::number(end.y, 'f', 1) + ")");
    ui->label_time->setText(QString::number(result.data.size()*dt, 'f', 1));
    ui->label_distance->setText(QString::number(AVec(end).length(), 'f', 1));
    ui->label_max_h->setText(QString::number(result.z_max, 'f', 1));
    ui->label_v_end->setText(QString::number(result.v_end.length(), 'f', 1));
    ui->label_alpha_end->setText(QString::number(rad_to_deg(alpha_end), 'f', 1) + "°");

    // Убираем предыдущий график
    if (! ui->btn_fixed->isChecked()){
        for (auto ser : chart->seriesList()){
            chart->removeSeries(ser);
        }
    } else {
        chart->seriesList().at(0)->setBaseColor(Qt::green);
    }

    // Добавляем точки
    series = new QScatter3DSeries;
    series->dataProxy()->addItems(result.data);
    series->setBaseColor(Qt::red);
    chart->addSeries(series);

    // Добавляем точку старта на график отдельным цветом
    start_point = new QScatter3DSeries;
    QScatterDataArray start_p_data;
    start_p_data << QVector3D(0.0, 0.0, 0.0);
    start_point->setBaseColor(Qt::black);
    start_point->setItemSize(series->itemSize()*1.2f);
    start_point->dataProxy()->addItems(start_p_data);
    chart->addSeries(start_point);

    // Добавляем точку цели на график отдельным цветом
    target_point = new QScatter3DSeries;
    QScatterDataArray target_p_data;
    target_p_data << QVector3D(target_x, 0.0, target_y);
    target_point->setBaseColor(Qt::darkRed);
    target_point->setItemSize(series->itemSize()*1.2f);
    target_point->dataProxy()->addItems(target_p_data);
    chart->addSeries(target_point);

    // Настраиваем оси и показываем график
    float new_range = std::max(std::max(abs(end.x), abs(end.y)), abs(result.z_max));
    new_range = std::max(std::max(abs(target_x), abs(target_y)), new_range);
    if (ui->btn_fixed->isChecked()){
        range = std::max(range, new_range);
    } else {
        range = new_range;
    }
    chart->axisX()->setRange(-range, range);
    chart->axisY()->setRange(0, range);
    chart->axisZ()->setRange(0, range);
    chart->setAspectRatio(2);
    chart->setHorizontalAspectRatio(2);
    chart->show();
}


float MainWindow::get_alpha(){return anchor_alpha + ui->edt_alpha->value()*step_alpha;}
float MainWindow::get_beta(){return anchor_beta + ui->edt_beta->value()*step_beta;}
float MainWindow::get_gamma(){return anchor_gamma + ui->edt_gamma->value()*step_gamma;}


void MainWindow::on_edt_alpha_valueChanged()
{
    ui->label_alpha->setText("Угол вертикальной наводки: (" + QString::number(get_alpha()) + "°)");
    //std::cout << anchor_alpha << " " << step_alpha << std::endl;
}


void MainWindow::on_edt_beta_valueChanged()
{
    ui->label_beta->setText("Угол горизонтальной наводки: (" + QString::number(get_beta()) + "°)");
}


void MainWindow::on_edt_gamma_valueChanged()
{
    ui->label_gamma->setText("Направление ветра: (" + QString::number(get_gamma()) + "°)");
}


// режим повышенной точности
void MainWindow::on_precise_alpha_clicked()
{
    if (ui->precise_alpha->isChecked()){
        anchor_alpha = get_alpha();
        step_alpha = 0.05;
        ui->edt_alpha->setValue(0);
    } else {
        ui->edt_alpha->setValue(int(get_alpha()));
        anchor_alpha = 0;
        step_alpha = 1.0;
        MainWindow::on_edt_alpha_valueChanged();
    }
}

void MainWindow::on_precise_beta_clicked()
{
    if (ui->precise_beta->isChecked()){
        anchor_beta = get_beta();
        step_beta = 0.05;
        ui->edt_beta->setValue(0);
    } else {
        ui->edt_beta->setValue(int(get_beta()));
        anchor_beta = 0;
        step_beta = 1.0;
        MainWindow::on_edt_beta_valueChanged();
    }
}

void MainWindow::on_precise_gamma_clicked()
{
    if (ui->precise_gamma->isChecked()){
        anchor_gamma = get_gamma();
        step_gamma = 0.1;
        ui->edt_gamma->setValue(0);
    } else {
        ui->edt_gamma->setValue(int(get_gamma()));
        anchor_gamma = 0;
        step_gamma = 1.0;
        MainWindow::on_edt_gamma_valueChanged();
    }
}


void MainWindow::on_pushButton_optimal_clicked()
{
    // Два способа ввода начальных данных: через пользовательский ввод и через специальную строку параметров (больше точность)
    if (!ui->btn_optres->isChecked()){
        v0 = ui->edt_v0->text().toFloat();
        alpha = deg_to_rad(get_alpha());
        beta = deg_to_rad(get_beta());
        u_value = ui->edt_u->text().toFloat();
        gamma = deg_to_rad(get_gamma());
        mu = ui->edt_mu->text().toFloat();
        m = ui->edt_m->text().toFloat();
        dt = ui->edt_dt->text().toFloat();
    } else {
        QStringList params_list = ui->edt_optres->text().split(";");
        v0 = params_list[0].toFloat();
        alpha = deg_to_rad(params_list[1].toFloat());
        beta = deg_to_rad(params_list[2].toFloat());
        u_value = params_list[3].toFloat();
        gamma = deg_to_rad(params_list[4].toFloat());
        mu = params_list[5].toFloat();
        m = params_list[6].toFloat();
        dt = params_list[7].toFloat();
    }
    target_x = ui->edt_target_x->text().toFloat();
    target_y = ui->edt_target_y->text().toFloat();
    Vec u = Vec(u_value, 0.0, gamma);

    ext_params ep;
    ep.dt = dt;
    ep.m = m;
    ep.mu = mu;
    ep.u = u;

    grad_params gp;
    gp.da = ui->edt_da->text().toFloat();
    gp.db = ui->edt_db->text().toFloat();
    gp.stepa = ui->edt_astep->text().toFloat();
    gp.stepb = ui->edt_bstep->text().toFloat();

    grad_return opt_params;
    opt_params = gradient_descent(gp, v0, alpha, beta, P(target_x, target_y, 0.0), ep);
    std::cout << opt_params.alpha << " " << opt_params.beta << " " << opt_params.func_value << std::endl;

    // ТОЧНОСТЬ!!
    ui->edt_alpha->setValue(rad_to_deg(opt_params.alpha));
    ui->edt_beta->setValue(rad_to_deg(opt_params.beta));
    QString params_string = QString::number(v0) + ";" + QString::number(rad_to_deg(opt_params.alpha), 'g', 5) + ";" + QString::number(rad_to_deg(opt_params.beta), 'g', 5) + "; "
            + QString::number(u_value) + ";" + QString::number(rad_to_deg(gamma), 'g', 5) + ";" + QString::number(mu) + ";" + QString::number(m) + ";" + QString::number(dt);
    ui->edt_optres->setText(params_string);

    std::cout << params_string.toStdString() << std::endl;

    start(false);
}

