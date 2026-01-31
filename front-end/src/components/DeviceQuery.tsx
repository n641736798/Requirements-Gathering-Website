import { useState, useEffect } from 'react';
import { queryDeviceData, DeviceDataPoint } from '../api/deviceApi';
import { formatTimestamp } from '../utils/format';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import './DeviceQuery.css';

export default function DeviceQuery() {
  const [deviceId, setDeviceId] = useState('');
  const [limit, setLimit] = useState(100);
  const [data, setData] = useState<DeviceDataPoint[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [autoRefresh, setAutoRefresh] = useState(false);
  const [refreshInterval, setRefreshInterval] = useState(5); // 秒

  const fetchData = async () => {
    if (!deviceId.trim()) {
      setError('请输入设备ID');
      return;
    }

    setLoading(true);
    setError(null);

    try {
      const response = await queryDeviceData(deviceId.trim(), limit);
      setData(response.data || []);
    } catch (err: any) {
      setError(err.response?.data?.message || err.message || '查询失败，请检查网络连接');
      setData([]);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    if (autoRefresh && deviceId.trim()) {
      const interval = setInterval(() => {
        fetchData();
      }, refreshInterval * 1000);

      return () => clearInterval(interval);
    }
  }, [autoRefresh, deviceId, refreshInterval]);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    fetchData();
  };

  // 准备图表数据
  const chartData = data.map((point) => ({
    timestamp: formatTimestamp(point.timestamp),
    ...Object.entries(point)
      .filter(([key]) => key !== 'timestamp')
      .reduce((acc, [key, value]) => {
        acc[key] = typeof value === 'number' ? value : parseFloat(String(value)) || 0;
        return acc;
      }, {} as Record<string, number>),
  }));

  // 获取所有指标名称（排除timestamp）
  const metricKeys = data.length > 0
    ? Object.keys(data[0]).filter((key) => key !== 'timestamp')
    : [];

  const colors = ['#8884d8', '#82ca9d', '#ffc658', '#ff7300', '#00ff00', '#ff00ff'];

  // 根据数据点数量动态设置X轴标签间隔
  // 数据点少于10个时显示所有标签，否则自动间隔
  const xAxisInterval = data.length <= 10 ? 0 : 'preserveStartEnd';

  return (
    <div className="device-query">
      <h2>设备数据查询</h2>

      <form onSubmit={handleSubmit} className="query-form">
        <div className="form-row">
          <div className="form-group">
            <label htmlFor="query_device_id">设备ID *</label>
            <input
              id="query_device_id"
              type="text"
              value={deviceId}
              onChange={(e) => setDeviceId(e.target.value)}
              placeholder="例如: ECG_10086"
              required
              disabled={loading}
            />
          </div>

          <div className="form-group">
            <label htmlFor="limit">数据条数</label>
            <input
              id="limit"
              type="number"
              value={limit}
              onChange={(e) => setLimit(Math.max(1, Math.min(1000, parseInt(e.target.value) || 100)))}
              min="1"
              max="1000"
              disabled={loading}
            />
          </div>

          <div className="form-group">
            <button type="submit" className="btn-query" disabled={loading}>
              {loading ? '查询中...' : '查询'}
            </button>
          </div>
        </div>

        <div className="auto-refresh-section">
          <label>
            <input
              type="checkbox"
              checked={autoRefresh}
              onChange={(e) => setAutoRefresh(e.target.checked)}
              disabled={!deviceId.trim() || loading}
            />
            自动刷新
          </label>
          {autoRefresh && (
            <div className="refresh-interval">
              <label>刷新间隔（秒）:</label>
              <input
                type="number"
                value={refreshInterval}
                onChange={(e) => setRefreshInterval(Math.max(1, parseInt(e.target.value) || 5))}
                min="1"
                max="60"
                disabled={loading}
              />
            </div>
          )}
        </div>
      </form>

      {error && <div className="error-message">{error}</div>}

      {data.length > 0 && (
        <div className="query-results">
          <div className="results-header">
            <h3>查询结果 ({data.length} 条)</h3>
          </div>

          {metricKeys.length > 0 && (
            <div className="chart-container">
              <ResponsiveContainer width="100%" height={450}>
                <LineChart data={chartData} margin={{ bottom: 100, right: 20, top: 20, left: 20 }}>
                  <CartesianGrid strokeDasharray="3 3" />
                  <XAxis
                    dataKey="timestamp"
                    angle={-30}
                    textAnchor="end"
                    height={120}
                    interval={xAxisInterval}
                    tick={{ fontSize: 12 }}
                    tickMargin={10}
                  />
                  <YAxis />
                  <Tooltip />
                  <Legend />
                  {metricKeys.map((key, index) => (
                    <Line
                      key={key}
                      type="monotone"
                      dataKey={key}
                      stroke={colors[index % colors.length]}
                      strokeWidth={2}
                      dot={{ r: 3 }}
                    />
                  ))}
                </LineChart>
              </ResponsiveContainer>
            </div>
          )}

          <div className="data-table-container">
            <table className="data-table">
              <thead>
                <tr>
                  <th>时间戳</th>
                  <th>时间</th>
                  {metricKeys.map((key) => (
                    <th key={key}>{key}</th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {data.map((point, index) => (
                  <tr key={index}>
                    <td>{point.timestamp}</td>
                    <td>{formatTimestamp(point.timestamp)}</td>
                    {metricKeys.map((key) => (
                      <td key={key}>{point[key]}</td>
                    ))}
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {!loading && !error && data.length === 0 && deviceId && (
        <div className="no-data">暂无数据</div>
      )}
    </div>
  );
}
