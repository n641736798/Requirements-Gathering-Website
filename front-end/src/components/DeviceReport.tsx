import { useState } from 'react';
import { reportDeviceData, DeviceReport as DeviceReportData } from '../api/deviceApi';
import { getCurrentTimestamp } from '../utils/format';
import './DeviceReport.css';

export default function DeviceReport() {
  const [deviceId, setDeviceId] = useState('');
  const [metrics, setMetrics] = useState<Record<string, string>>({
    heart_rate: '',
    spo2: '',
  });
  const [loading, setLoading] = useState(false);
  const [message, setMessage] = useState<{ type: 'success' | 'error'; text: string } | null>(null);

  const handleMetricChange = (key: string, value: string) => {
    setMetrics((prev) => ({
      ...prev,
      [key]: value,
    }));
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setLoading(true);
    setMessage(null);

    try {
      // 验证设备ID
      if (!deviceId.trim()) {
        throw new Error('设备ID不能为空');
      }

      // 构建指标对象，过滤空值并转换为数字
      const metricsObj: Record<string, number | string> = {};
      Object.entries(metrics).forEach(([key, value]) => {
        if (value.trim()) {
          const numValue = parseFloat(value);
          metricsObj[key] = isNaN(numValue) ? value : numValue;
        }
      });

      if (Object.keys(metricsObj).length === 0) {
        throw new Error('至少需要提供一个指标');
      }

      const reportData: DeviceReportData = {
        device_id: deviceId.trim(),
        timestamp: getCurrentTimestamp(),
        metrics: metricsObj,
      };

      const response = await reportDeviceData(reportData);

      if (response.code === 0) {
        setMessage({ type: 'success', text: '数据上报成功！' });
        // 清空表单
        setDeviceId('');
        setMetrics({ heart_rate: '', spo2: '' });
      } else {
        throw new Error(response.message || '上报失败');
      }
    } catch (error: any) {
      setMessage({
        type: 'error',
        text: error.response?.data?.message || error.message || '上报失败，请检查网络连接',
      });
    } finally {
      setLoading(false);
    }
  };

  const addMetricField = () => {
    const newKey = `metric_${Date.now()}`;
    setMetrics((prev) => ({
      ...prev,
      [newKey]: '',
    }));
  };

  const removeMetricField = (key: string) => {
    setMetrics((prev) => {
      const newMetrics = { ...prev };
      delete newMetrics[key];
      return newMetrics;
    });
  };

  return (
    <div className="device-report">
      <h2>设备数据上报</h2>
      <form onSubmit={handleSubmit} className="report-form">
        <div className="form-group">
          <label htmlFor="device_id">设备ID *</label>
          <input
            id="device_id"
            type="text"
            value={deviceId}
            onChange={(e) => setDeviceId(e.target.value)}
            placeholder="例如: ECG_10086"
            required
            disabled={loading}
          />
        </div>

        <div className="metrics-section">
          <div className="metrics-header">
            <label>指标数据 *</label>
            <button type="button" onClick={addMetricField} className="btn-add" disabled={loading}>
              + 添加指标
            </button>
          </div>

          {Object.entries(metrics).map(([key, value]) => (
            <div key={key} className="metric-row">
              <input
                type="text"
                value={key}
                onChange={(e) => {
                  const newMetrics = { ...metrics };
                  delete newMetrics[key];
                  newMetrics[e.target.value] = value;
                  setMetrics(newMetrics);
                }}
                placeholder="指标名称"
                className="metric-key"
                disabled={loading}
              />
              <input
                type="text"
                value={value}
                onChange={(e) => handleMetricChange(key, e.target.value)}
                placeholder="指标值"
                className="metric-value"
                disabled={loading}
              />
              {Object.keys(metrics).length > 1 && (
                <button
                  type="button"
                  onClick={() => removeMetricField(key)}
                  className="btn-remove"
                  disabled={loading}
                >
                  删除
                </button>
              )}
            </div>
          ))}
        </div>

        {message && (
          <div className={`message message-${message.type}`}>{message.text}</div>
        )}

        <button type="submit" className="btn-submit" disabled={loading}>
          {loading ? '上报中...' : '提交上报'}
        </button>
      </form>
    </div>
  );
}
