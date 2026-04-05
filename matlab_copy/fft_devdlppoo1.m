function [estimated_freq, results] = adaptive_fft_frequency_estimation()
    % 自适应采样率FFT频率估计
    % 输出:
    %   estimated_freq - 最终估计的频率
    %   results - 迭代结果记录表
    
    clear; close all; clc;
    
    %% 可调参数设置
    params.base_sampling_rate = 3.6e6;      % 基础采样率 3.6 MHz
    params.offset_frequency = 10000;         % 每轮采样偏移 10 kHz
    params.fft_length = 1024;                 % FFT长度
    params.freq_min = 1000;                    % 信号频率下限 1 kHz
    params.freq_max = 1e6;                     % 信号频率上限 1 MHz
    params.error_threshold = 5.0;              % 误差阈值百分比
    params.max_iterations = 360;                 % 最大迭代次数
    params.duration = 0.01;                     % 信号持续时间 (s)
    
    %% 显示参数设置
    fprintf('========================================\n');
    fprintf('自适应采样率FFT频率估计仿真\n');
    fprintf('========================================\n');
    fprintf('参数设置:\n');
    fprintf('  基础采样率: %.3f MHz\n', params.base_sampling_rate/1e6);
    fprintf('  采样偏移: %.3f kHz\n', params.offset_frequency/1e3);
    fprintf('  FFT长度: %d\n', params.fft_length);
    fprintf('  频率范围: %.1f kHz - %.1f MHz\n', params.freq_min/1e3, params.freq_max/1e6);
    fprintf('  误差阈值: %.1f%%\n', params.error_threshold);
    fprintf('========================================\n');
    
    %% 生成随机信号频率
    signal_frequency = params.freq_min + rand() * (params.freq_max - params.freq_min);
    % signal_frequency = 1000;
    fprintf('\n目标信号频率: %.2f Hz (%.3f kHz)\n', signal_frequency, signal_frequency/1000);
    
    %% 初始化
    current_sampling_rate = params.base_sampling_rate;
    iteration = 0;
    results = [];
    
    %% 主循环
    while iteration < params.max_iterations
        iteration = iteration + 1;
        fprintf('\n----- 迭代 %d -----\n', iteration);
        fprintf('当前采样率: %.3f MHz (%.0f Hz)\n', current_sampling_rate/1e6, current_sampling_rate);
        
        % 生成信号
        [signal_data, t] = generate_signal(signal_frequency, current_sampling_rate, params.duration);
        
        % 进行FFT分析
        [fft_magnitude, freq_axis, main_freq, main_amplitude, side_amplitude, main_idx, side_idx] = ...
            analyze_fft(signal_data, current_sampling_rate, params.fft_length);
        
        % 计算误差
        error_percent = calculate_error(main_amplitude, side_amplitude);
        
        % 记录结果
        result = struct(...
            'iteration', iteration, ...
            'sampling_rate', current_sampling_rate, ...
            'main_freq', main_freq, ...
            'main_amplitude', main_amplitude, ...
            'side_amplitude', side_amplitude, ...
            'error_percent', error_percent, ...
            'main_idx', main_idx, ...
            'side_idx', side_idx);
        results = [results; result];
        
        % 输出当前迭代的详细信息
        fprintf('  主峰FFT频率: %.2f Hz (索引: %d)\n', main_freq, main_idx);
        fprintf('  主峰幅值: %.4f\n', main_amplitude);
        fprintf('  旁峰FFT频率: %.2f Hz (索引: %d)\n', freq_axis(side_idx), side_idx);
        fprintf('  旁峰幅值: %.4f\n', side_amplitude);
        fprintf('  幅值误差: %.4f%%\n', error_percent);
        fprintf('  频率分辨率: %.2f Hz\n', current_sampling_rate/params.fft_length);
        
        % 检查是否达到误差要求
        if error_percent < params.error_threshold
            % 计算两峰中间的频率
            mid_freq = (freq_axis(main_idx) + freq_axis(side_idx)) / 2;
            
            fprintf('\n');
            fprintf('════════════════════════════════════════════\n');
            fprintf('✓ 误差小于%.1f%%，达到收敛条件！\n', params.error_threshold);
            fprintf('════════════════════════════════════════════\n');
            fprintf('\n当前状态报告:\n');
            fprintf('----------------------------------------\n');
            fprintf('【系统参数】\n');
            fprintf('  当前采样率: %.3f MHz\n', current_sampling_rate/1e6);
            fprintf('  采样偏移量: %.3f kHz\n', params.offset_frequency/1e3);
            fprintf('  FFT长度: %d\n', params.fft_length);
            fprintf('  频率分辨率: %.2f Hz\n', current_sampling_rate/params.fft_length);
            fprintf('\n【信号参数】\n');
            fprintf('  真实信号频率: %.2f Hz (%.3f kHz)\n', signal_frequency, signal_frequency/1000);
            fprintf('  信号持续时间: %.3f ms\n', params.duration*1000);
            fprintf('  采样点数: %d\n', length(signal_data));
            fprintf('\n【FFT分析结果】\n');
            fprintf('  主峰索引: %d\n', main_idx);
            fprintf('  主峰频率: %.2f Hz\n', main_freq);
            fprintf('  主峰幅值: %.4f\n', main_amplitude);
            fprintf('  旁峰索引: %d\n', side_idx);
            fprintf('  旁峰频率: %.2f Hz\n', freq_axis(side_idx));
            fprintf('  旁峰幅值: %.4f\n', side_amplitude);
            fprintf('  幅值误差: %.4f%%\n', error_percent);
            fprintf('\n【频率估计结果】\n');
            fprintf('  中点估计频率: %.2f Hz\n', mid_freq);
            fprintf('  估计误差: %.4f Hz (%.4f%%)\n', ...
                abs(mid_freq - signal_frequency), ...
                abs(mid_freq - signal_frequency)/signal_frequency*100);
            fprintf('----------------------------------------\n');
            fprintf('\n【迭代历史摘要】\n');
            fprintf('  总迭代次数: %d\n', iteration);
            fprintf('  起始采样率: %.3f MHz\n', params.base_sampling_rate/1e6);
            fprintf('  最终采样率: %.3f MHz\n', current_sampling_rate/1e6);
            fprintf('  采样率变化: %.3f MHz\n', (params.base_sampling_rate - current_sampling_rate)/1e6);
            fprintf('----------------------------------------\n');
            
            % 绘制结果
            plot_results(signal_data, t, fft_magnitude, freq_axis, results, signal_frequency, params, mid_freq);
            
            estimated_freq = mid_freq;
            return;
        end
        
        % 调整采样率
        current_sampling_rate = current_sampling_rate - params.offset_frequency;
        
        % 检查采样率是否过低
        if current_sampling_rate < signal_frequency * 2.1
            fprintf('\n⚠ 警告：采样率接近奈奎斯特极限 (%.2f * 2 = %.2f Hz)\n', ...
                signal_frequency, signal_frequency*2);
            fprintf('当前采样率: %.2f Hz\n', current_sampling_rate);
            
            if current_sampling_rate <= signal_frequency * 2
                fprintf('✗ 采样率低于奈奎斯特频率，停止迭代\n');
                break;
            end
        end
    end
    
    fprintf('\n⚠ 达到最大迭代次数或采样率过低，未收敛到指定误差\n');
    estimated_freq = [];
    
    % 绘制最后一次的结果
    if ~isempty(results)
        plot_results(signal_data, t, fft_magnitude, freq_axis, results, signal_frequency, params, []);
    end
end

function [signal_data, t] = generate_signal(frequency, sampling_rate, duration)
    % 生成正弦信号
    t = 0:1/sampling_rate:duration;
    signal_data = sin(2 * pi * frequency * t);
end

function [fft_magnitude, freq_axis, main_freq, main_amplitude, side_amplitude, main_idx, side_idx] = ...
    analyze_fft(signal_data, sampling_rate, fft_length)
    % 进行FFT分析并提取主峰和旁峰信息
    
    % 确保信号长度足够
    if length(signal_data) < fft_length
        signal_data = [signal_data, zeros(1, fft_length - length(signal_data))];
    else
        signal_data = signal_data(1:fft_length);
    end
    
    % 应用汉宁窗减少频谱泄漏
    window = hann(fft_length)';
    signal_windowed = signal_data .* window;
    
    % 进行FFT
    fft_result = fft(signal_windowed, fft_length);
    fft_magnitude = abs(fft_result(1:fft_length/2));
    
    % 计算频率轴
    freq_axis = (0:fft_length/2-1) * sampling_rate / fft_length;
    
    % 找到主峰（忽略0频）
    [main_amplitude, main_idx] = max(fft_magnitude(2:end));
    main_idx = main_idx + 1;  % 调整索引以包含0频
    main_freq = freq_axis(main_idx);
    
    % 找到旁峰（主峰+1，考虑边界）
    if main_idx < length(fft_magnitude)
        side_idx = main_idx + 1;
    else
        side_idx = main_idx - 1;
    end
    side_amplitude = fft_magnitude(side_idx);
end

function error_percent = calculate_error(main_amplitude, side_amplitude)
    % 计算主峰和旁峰的幅值误差百分比
    error_percent = abs(main_amplitude - side_amplitude) / main_amplitude * 100;
end

function plot_results(signal_data, t, fft_magnitude, freq_axis, results, true_freq, params, est_freq)
    % 绘制结果图表
    
    figure('Position', [100, 100, 1400, 900]);
    
    % 时域信号
    subplot(2, 3, 1);
    plot(t(1:min(500, length(t)))*1e6, signal_data(1:min(500, length(t))));
    xlabel('时间 (μs)');
    ylabel('幅值');
    title('时域信号 (前500点)');
    grid on;
    
    % FFT频谱
    subplot(2, 3, 2);
    plot(freq_axis/1000, fft_magnitude, 'b-', 'LineWidth', 1.5);
    xlabel('频率 (kHz)');
    ylabel('幅值');
    title('FFT频谱');
    grid on;
    
    % 标记主峰和旁峰
    [~, main_idx] = max(fft_magnitude(2:end));
    main_idx = main_idx + 1;
    if main_idx < length(fft_magnitude)
        side_idx = main_idx + 1;
    else
        side_idx = main_idx - 1;
    end
    
    hold on;
    plot(freq_axis(main_idx)/1000, fft_magnitude(main_idx), 'ro', 'MarkerSize', 10, 'LineWidth', 2);
    plot(freq_axis(side_idx)/1000, fft_magnitude(side_idx), 'gs', 'MarkerSize', 8, 'LineWidth', 2);
    legend('频谱', '主峰', '旁峰');
    hold off;
    
    % 主峰附近放大图
    subplot(2, 3, 3);
    start_idx = max(1, main_idx - 15);
    end_idx = min(length(freq_axis), main_idx + 15);
    
    plot(freq_axis(start_idx:end_idx)/1000, fft_magnitude(start_idx:end_idx), 'b-o', 'LineWidth', 1.5);
    xlabel('频率 (kHz)');
    ylabel('幅值');
    title('主峰附近频谱 (放大)');
    grid on;
    
    hold on;
    plot(freq_axis(main_idx)/1000, fft_magnitude(main_idx), 'ro', 'MarkerSize', 10, 'LineWidth', 2);
    plot(freq_axis(side_idx)/1000, fft_magnitude(side_idx), 'gs', 'MarkerSize', 8, 'LineWidth', 2);
    
    if ~isempty(est_freq)
        xline(est_freq/1000, 'r--', 'LineWidth', 2, 'Label', '估计频率');
    end
    xline(true_freq/1000, 'g--', 'LineWidth', 2, 'Label', '真实频率');
    hold off;
    
    % 迭代误差变化
    subplot(2, 3, 4);
    iterations = [results.iteration];
    errors = [results.error_percent];
    plot(iterations, errors, 'r-o', 'LineWidth', 2, 'MarkerSize', 8);
    hold on;
    yline(params.error_threshold, 'g--', 'LineWidth', 2);
    xlabel('迭代次数');
    ylabel('误差百分比 (%)');
    title('迭代误差变化');
    legend('误差', sprintf('阈值 (%.1f%%)', params.error_threshold));
    grid on;
    
    % 主峰频率变化
    subplot(2, 3, 5);
    main_freqs = [results.main_freq];
    plot(iterations, main_freqs/1000, 'b-o', 'LineWidth', 2, 'MarkerSize', 8);
    hold on;
    yline(true_freq/1000, 'r--', 'LineWidth', 2);
    xlabel('迭代次数');
    ylabel('主峰频率 (kHz)');
    title('主峰频率变化');
    legend('估计频率', sprintf('真实频率 (%.2f kHz)', true_freq/1000));
    grid on;
    
    % 采样率变化
    subplot(2, 3, 6);
    sampling_rates = [results.sampling_rate];
    plot(iterations, sampling_rates/1e6, 'm-o', 'LineWidth', 2, 'MarkerSize', 8);
    xlabel('迭代次数');
    ylabel('采样率 (MHz)');
    title('采样率变化');
    grid on;
    
    % 添加收敛信息
    if ~isempty(est_freq)
        sgtitle(sprintf('自适应采样率FFT分析结果 (真实: %.2f kHz | 估计: %.2f kHz | 误差: %.3f%%)', ...
            true_freq/1000, est_freq/1000, abs(est_freq-true_freq)/true_freq*100));
    else
        sgtitle(sprintf('自适应采样率FFT分析结果 (真实频率: %.2f kHz)', true_freq/1000));
    end
    
    % 调整布局
    set(gcf, 'Color', 'white');
end

function run_multiple_simulations()
    % 运行多次仿真以测试算法鲁棒性
    
    num_simulations = 10;
    fprintf('\n========================================\n');
    fprintf('运行 %d 次仿真统计\n', num_simulations);
    fprintf('========================================\n');
    
    success_count = 0;
    total_error = 0;
    
    for i = 1:num_simulations
        fprintf('\n════════════════════════════════════════════\n');
        fprintf('仿真 %d/%d\n', i, num_simulations);
        fprintf('════════════════════════════════════════════\n');
        
        [est_freq, results] = adaptive_fft_frequency_estimation();
        
        if ~isempty(est_freq) && ~isempty(results)
            success_count = success_count + 1;
            % 这里需要获取真实频率，但在当前实现中无法直接获取
            % 实际应用中可以从全局变量或修改函数返回值获取
        end
        
        % 暂停一下让用户看清结果
        if i < num_simulations
            fprintf('\n按任意键继续下一次仿真...\n');
            pause;
        end
    end
    
    fprintf('\n========================================\n');
    fprintf('统计结果:\n');
    fprintf('  成功收敛次数: %d/%d (%.1f%%)\n', success_count, num_simulations, success_count/num_simulations*100);
    fprintf('========================================\n');
end