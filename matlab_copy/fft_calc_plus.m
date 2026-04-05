%% ========================================================================
% FFT栅栏效应校正算法仿真 - MATLAB精简版
% 核心：通过向下微调采样率，使FFT幅度谱的最高峰与相邻峰幅度差在x%以内，
%      利用两峰中间的频率计算真实频率。
% =========================================================================

clear; close all; clc;

%% ==================== 用户配置参数区域 ====================
% 采样率参数
config.fs_nominal = 3.6e6;          % 基础采样率 (Hz)
config.fs_step = 10000;              % 采样率调整步进 (Hz)
config.fs_min = 1.0e6;              % 最小采样率 (Hz)

% FFT参数
config.fft_points = 1024;            % FFT点数
config.max_amp_diff_percent = 5.0;   % 误差阈值百分比 (%)
config.duration = 0.01;              % 信号持续时间 (秒)

% ADC量化参数 (设置adc_bits为[]表示不量化)
config.adc_bits = 8;                % 量化位数: 8, 12, 16, []表示不量化
config.adc_full_scale = 1.0;         % ADC满量程幅度

% 信号参数范围
config.amplitude_min = 0.1;          % 最小幅度
config.amplitude_max = 1.0;          % 最大幅度
config.snr_min_db = 20.0;            % 最小SNR (dB)
config.snr_max_db = 40.0;            % 最大SNR (dB)
config.freq_min = 1000;              % 最小频率 (Hz)
config.freq_max = 2e4;               % 最大频率 (Hz)
config.waveform_types = {'sine', 'square', 'triangle'}; % 波形类型

% 仿真配置
config.num_simulations = 4000;        % 总仿真次数
config.num_workers = 16;              % 并行线程数
config.fixed_seed = 42;              % 固定随机种子

% 输出配置
config.save_plots = true;            % 是否保存图表
config.output_dir = './simulation_results'; % 输出文件夹
%% ==================== 程序开始 ====================

% 显示配置信息
fprintf('\n========================================\n');
fprintf('FFT栅栏效应校正算法仿真\n');
fprintf('========================================\n');
fprintf('采样率: %.3f MHz | 步进: %.0f kHz | FFT: %d\n', ...
    config.fs_nominal/1e6, config.fs_step/1e3, config.fft_points);
fprintf('误差阈值: %.1f%% | 仿真次数: %d | 线程: %d\n', ...
    config.max_amp_diff_percent, config.num_simulations, config.num_workers);
if ~isempty(config.adc_bits)
    fprintf('ADC: %d bit | SNR: %.0f-%.0f dB\n', config.adc_bits, config.snr_min_db, config.snr_max_db);
else
    fprintf('ADC: 无 | SNR: %.0f-%.0f dB\n', config.snr_min_db, config.snr_max_db);
end
fprintf('频率: %.0f-%.0f kHz | 幅度: %.1f-%.1f\n', ...
    config.freq_min/1e3, config.freq_max/1e3, config.amplitude_min, config.amplitude_max);
fprintf('========================================\n');

% 创建输出目录
timestamp = datestr(now, 'yyyymmdd_HHMMSS');
output_dir = fullfile(config.output_dir, ['run_' timestamp]);
if config.save_plots
    mkdir(output_dir);
    mkdir(fullfile(output_dir, 'csv'));
    mkdir(fullfile(output_dir, 'json'));
    mkdir(fullfile(output_dir, 'plots'));
    save_config_json(config, output_dir);
    fprintf('输出目录: %s\n', output_dir);
end

% 生成随机参数
rng(config.fixed_seed);
params_list = generate_params(config);

% 运行仿真
fprintf('\n开始仿真...\n');
total_start = tic;
results = run_simulations(config, params_list);
total_time_ms = toc(total_start) * 1000;

% 统计分析并显示结果
stats = compute_stats(results);
display_results(stats, total_time_ms);

% 保存结果
if config.save_plots
    save_results(results, stats, config, total_time_ms, output_dir);
    generate_plots(results, output_dir);
    fprintf('\n结果已保存到: %s\n', output_dir);
end

fprintf('\n仿真完成！\n');

fprintf('\n仿真完成！\n');

%% ==================== 清理内存 ====================
fprintf('清理内存...\n');

% 清理大型工作变量
clear results params_list
clear stats
clear total_start

% 关闭所有隐藏的图形（如果还有未关闭的）
close all hidden

% 关闭并行池
if ~isempty(gcp('nocreate'))
    pool = gcp('nocreate');
    delete(pool);
    fprintf('并行池已关闭\n');
end

% 强制垃圾回收
pack
fprintf('内存清理完成\n');

%% ==================== 核心函数 ====================

function params = generate_params(config)
    for i = 1:config.num_simulations
        params(i).freq = config.freq_min + rand() * (config.freq_max - config.freq_min);
        idx = randi(length(config.waveform_types));
        params(i).waveform = config.waveform_types{idx};
        params(i).amplitude = config.amplitude_min + rand() * (config.amplitude_max - config.amplitude_min);
        params(i).snr_db = config.snr_min_db + rand() * (config.snr_max_db - config.snr_min_db);
    end
end

function results = run_simulations(config, params)
    if config.num_workers > 1 && license('test', 'Distrib_Computing_Toolbox')
        if isempty(gcp('nocreate'))
            parpool('local', min(config.num_workers, feature('numcores')));
        end
        results_temp = cell(config.num_simulations, 1);
        parfor i = 1:config.num_simulations
            if ~isempty(config.fixed_seed), rng(config.fixed_seed + i); end
            results_temp{i} = run_single(i, params(i), config);
        end
        for i = 1:config.num_simulations, results(i) = results_temp{i}; end
    else
        for i = 1:config.num_simulations
            if ~isempty(config.fixed_seed), rng(config.fixed_seed + i); end
            results(i) = run_single(i, params(i), config);
            if mod(i, 20) == 0, fprintf(' 进度: %d/%d\n', i, config.num_simulations); end
        end
    end
end

function result = run_single(id, param, config)
    start_time = tic;
    [est_freq, success, iterations, final_err] = adaptive_fft(param.freq, ...
        param.waveform, param.amplitude, param.snr_db, config);
    result = struct('sim_id', id, 'true_freq', param.freq, 'waveform', param.waveform, ...
        'amplitude', param.amplitude, 'snr_db', param.snr_db, 'estimated_freq', est_freq, ...
        'success', success, 'iterations', iterations, 'final_error', final_err, 'elapsed_ms', toc(start_time)*1000);
end

function [est_freq, success, iter, final_err] = adaptive_fft(freq, waveform, amp, snr, config)
    fs = config.fs_nominal;
    max_iter = floor((config.fs_nominal - config.fs_min) / config.fs_step);
    
    for iter = 1:max_iter
        signal = gen_signal(freq, fs, config.duration, waveform, amp, snr);
        if ~isempty(config.adc_bits), signal = quantize(signal, config.adc_bits, config.adc_full_scale); end
        
        [mag, f_axis, main_f, main_a, side_a, main_idx, side_idx] = analyze_fft(signal, fs, config.fft_points);
        
        err = abs(main_a - side_a) / max(main_a, eps) * 100;
        
        if err < config.max_amp_diff_percent
            est_freq = (f_axis(main_idx) + f_axis(side_idx)) / 2;
            success = true;
            final_err = err;
            return;
        end
        
        fs = fs - config.fs_step;
        if fs < freq * 2.1, break; end
    end
    
    est_freq = []; success = false; final_err = err;
end

function signal = gen_signal(freq, fs, dur, waveform, amp, snr)
    t = 0:1/fs:dur;
    switch waveform
        case 'sine', signal = amp * sin(2*pi*freq*t);
        case 'square', signal = amp * square(2*pi*freq*t);
        case 'triangle', signal = amp * sawtooth(2*pi*freq*t, 0.5);
        otherwise, signal = amp * sin(2*pi*freq*t);
    end
    if ~isempty(snr)
        sig_pow = mean(signal.^2);
        noise_pow = sig_pow / (10^(snr/10));
        signal = signal + sqrt(noise_pow) * randn(size(signal));
    end
end

function sig_q = quantize(signal, bits, full_scale)
    ranges = containers.Map({8,12,16}, {[0,255],[0,4095],[0,65535]});
    if ~isKey(ranges, bits), sig_q = signal; return; end
    range_val = ranges(bits);
    normalized = (signal / full_scale + 1) / 2;
    quantized = round(normalized * (range_val(2)-range_val(1)) + range_val(1));
    quantized = max(range_val(1), min(range_val(2), quantized));
    sig_q = (quantized - range_val(1)) / (range_val(2)-range_val(1)) * 2 - 1;
end

function [mag, f_axis, main_f, main_a, side_a, main_idx, side_idx] = analyze_fft(signal, fs, nfft)
    if length(signal) < nfft
        signal = [signal, zeros(1, nfft-length(signal))];
    else
        signal = signal(1:nfft);
    end
    windowed = signal .* hann(nfft)';
    fft_res = fft(windowed, nfft);
    mag = abs(fft_res(1:nfft/2));
    f_axis = (0:nfft/2-1) * fs / nfft;
    [main_a, main_idx] = max(mag(2:end));
    main_idx = main_idx + 1;
    main_f = f_axis(main_idx);
    side_idx = min(main_idx + 1, length(mag));
    side_a = mag(side_idx);
end

function stats = compute_stats(results)
    stats.total = length(results);
    stats.successful = sum([results.success]);
    stats.success_rate = stats.successful / stats.total * 100;
    stats.mean_iterations = mean([results.iterations]);
    
    errors = [];
    for r = results
        if r.success && ~isempty(r.estimated_freq)
            errors = [errors, abs(r.estimated_freq - r.true_freq) / r.true_freq * 100];
        end
    end
    if ~isempty(errors)
        stats.mean_err = mean(errors);
        stats.std_err = std(errors);
        stats.max_err = max(errors);
        stats.min_err = min(errors);
    else
        stats.mean_err = []; stats.std_err = []; stats.max_err = []; stats.min_err = [];
    end
    
    waveforms = unique({results.waveform});
    for w = waveforms'
        idx = strcmp({results.waveform}, w{1}) & [results.success];
        if any(idx)
            errs = [];
            for j = find(idx)
                errs = [errs, abs(results(j).estimated_freq - results(j).true_freq) / results(j).true_freq * 100];
            end
            stats.waveform.(w{1}) = struct('count', sum(idx), 'mean', mean(errs), 'std', std(errs));
        end
    end
end

function display_results(stats, total_ms)
    fprintf('\n========================================\n');
    fprintf('仿真结果汇总\n');
    fprintf('========================================\n');
    fprintf('总次数: %d | 成功: %d (%.1f%%) | 平均迭代: %.1f\n', ...
        stats.total, stats.successful, stats.success_rate, stats.mean_iterations);
    fprintf('总耗时: %.2f ms | 平均耗时: %.2f ms\n', total_ms, total_ms/stats.total);
    
    if ~isempty(stats.mean_err)
        fprintf('\n频率误差统计:\n');
        fprintf('  平均: %.4f%% | 标准差: %.4f%%\n', stats.mean_err, stats.std_err);
        fprintf('  最大: %.4f%% | 最小: %.4f%%\n', stats.max_err, stats.min_err);
    end
    
    fprintf('\n按波形统计:\n');
    w_names = fieldnames(stats.waveform);
    for i = 1:length(w_names)
        w = stats.waveform.(w_names{i});
        fprintf('  %s: %d次, 误差 %.4f%% ± %.4f%%\n', upper(w_names{i}), w.count, w.mean, w.std);
    end
    fprintf('========================================\n');
end

function save_results(results, stats, config, total_ms, out_dir)
    % 保存CSV
    csv_path = fullfile(out_dir, 'csv', 'results.csv');
    fid = fopen(csv_path, 'w');
    fprintf(fid, 'id,true_freq_hz,waveform,amplitude,snr_db,est_freq_hz,error_pct,success,iterations,elapsed_ms\n');
    for r = results
        if r.success && ~isempty(r.estimated_freq)
            err_pct = abs(r.estimated_freq - r.true_freq) / r.true_freq * 100;
            fprintf(fid, '%d,%.2f,%s,%.4f,%.2f,%.2f,%.6f,%d,%d,%.2f\n', ...
                r.sim_id, r.true_freq, r.waveform, r.amplitude, r.snr_db, ...
                r.estimated_freq, err_pct, r.success, r.iterations, r.elapsed_ms);
        else
            fprintf(fid, '%d,%.2f,%s,%.4f,%.2f,,,%d,%d,%.2f\n', ...
                r.sim_id, r.true_freq, r.waveform, r.amplitude, r.snr_db, ...
                r.success, r.iterations, r.elapsed_ms);
        end
    end
    fclose(fid);
    
    % 保存JSON摘要
    json_path = fullfile(out_dir, 'json', 'summary.json');
    summary = struct('timestamp', datestr(now), 'total_time_ms', total_ms, ...
        'total', stats.total, 'successful', stats.successful, 'success_rate', stats.success_rate, ...
        'mean_iterations', stats.mean_iterations, 'mean_error', stats.mean_err, ...
        'std_error', stats.std_err, 'max_error', stats.max_err, 'min_error', stats.min_err);
    fid = fopen(json_path, 'w');
    fprintf(fid, '%s', jsonencode(summary, 'PrettyPrint', true));
    fclose(fid);
end

function save_config_json(config, out_dir)
    cfg = struct('fs_nominal', config.fs_nominal, 'fs_step', config.fs_step, ...
        'fft_points', config.fft_points, 'max_amp_diff_percent', config.max_amp_diff_percent, ...
        'duration', config.duration, 'adc_bits', config.adc_bits, ...
        'num_simulations', config.num_simulations, 'num_workers', config.num_workers);
    fid = fopen(fullfile(out_dir, 'json', 'config.json'), 'w');
    fprintf(fid, '%s', jsonencode(cfg, 'PrettyPrint', true));
    fclose(fid);
end

function generate_plots(results, out_dir)
    plots_dir = fullfile(out_dir, 'plots');
    
    % 收集误差数据
    errors = []; snr_vals = []; freq_vals = [];
    for r = results
        if r.success && ~isempty(r.estimated_freq)
            e = abs(r.estimated_freq - r.true_freq) / r.true_freq * 100;
            errors = [errors, e];
            snr_vals = [snr_vals, r.snr_db];
            freq_vals = [freq_vals, r.true_freq/1000];
        end
    end
    
    if isempty(errors), return; end
    
    % 图1: 误差分布
    figure('Visible', 'off');
    histogram(errors, 30, 'FaceColor', 'b', 'FaceAlpha', 0.7);
    xlabel('频率误差 (%)'); ylabel('频次'); title('频率误差分布'); grid on;
    saveas(gcf, fullfile(plots_dir, 'error_dist.png')); close;
    
    % 图2: 误差vs SNR
    figure('Visible', 'off');
    scatter(snr_vals, errors, 20, 'b', 'filled');
    xlabel('SNR (dB)'); ylabel('频率误差 (%)'); title('误差 vs SNR'); grid on;
    p = polyfit(snr_vals, errors, 1);
    hold on; plot(sort(snr_vals), polyval(p, sort(snr_vals)), 'r--');
    legend('数据', sprintf('趋势 (斜率:%.4f)', p(1))); hold off;
    saveas(gcf, fullfile(plots_dir, 'error_vs_snr.png')); close;
    
    % 图3: 误差vs频率
    figure('Visible', 'off');
    scatter(freq_vals, errors, 20, 'g', 'filled');
    xlabel('频率 (kHz)'); ylabel('频率误差 (%)'); title('误差 vs 频率'); grid on;
    saveas(gcf, fullfile(plots_dir, 'error_vs_freq.png')); close;
    
    % 图4: 按波形箱线图 (修复版本)
    figure('Visible', 'off');
    waveforms = unique({results.waveform});
    box_data = [];
    group_labels = [];
    
    for i = 1:length(waveforms)
        w = waveforms{i};
        for r = results
            if r.success && strcmp(r.waveform, w) && ~isempty(r.estimated_freq)
                e = abs(r.estimated_freq - r.true_freq) / r.true_freq * 100;
                box_data = [box_data; e];
                group_labels = [group_labels; i];
            end
        end
    end
    
    if ~isempty(box_data)
        boxplot(box_data, group_labels);
        set(gca, 'XTickLabel', waveforms);
        xlabel('波形类型');
        ylabel('频率误差 (%)');
        title('不同波形类型的频率误差分布');
        grid on;
        saveas(gcf, fullfile(plots_dir, 'error_by_waveform.png'));
        close(gcf);
    end
    
    % 图5: 成功率饼图
    figure('Visible', 'off');
    suc = sum([results.success]); fail = length(results) - suc;
    pie([suc, fail], {sprintf('成功(%d)', suc), sprintf('失败(%d)', fail)});
    title('成功率'); colormap([0.3,0.7,0.9; 0.9,0.4,0.4]);
    saveas(gcf, fullfile(plots_dir, 'success_rate.png')); close;
    
    % 图6: 迭代次数分布
    figure('Visible', 'off');
    histogram([results.iterations], 20, 'FaceColor', 'm', 'FaceAlpha', 0.7);
    xlabel('迭代次数'); ylabel('频次'); title('迭代次数分布'); grid on;
    hold on; xline(mean([results.iterations]), 'r--');
    legend('频次', sprintf('均值:%.1f', mean([results.iterations]))); hold off;
    saveas(gcf, fullfile(plots_dir, 'iter_dist.png')); close;
end