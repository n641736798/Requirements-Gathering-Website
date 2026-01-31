import { useState, useEffect } from 'react';
import { queryRequirements, Requirement } from '../api/requirementApi';
import './RequirementQuery.css';

function getWillingToPayLabel(v: number | null): string {
  if (v === null || v < 0) return '未填';
  if (v === 1) return '愿意';
  return '不愿意';
}

export default function RequirementQuery() {
  const [data, setData] = useState<Requirement[]>([]);
  const [total, setTotal] = useState(0);
  const [page, setPage] = useState(1);
  const [limit] = useState(100);
  const [willingToPay, setWillingToPay] = useState<number | ''>('');
  const [keyword, setKeyword] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [expandedId, setExpandedId] = useState<number | null>(null);

  const [searchTrigger, setSearchTrigger] = useState(0);

  const fetchData = async () => {
    setLoading(true);
    setError(null);

    try {
      const params: { page: number; limit: number; willing_to_pay?: number; keyword?: string } = {
        page,
        limit,
      };
      if (willingToPay !== '') params.willing_to_pay = willingToPay;
      if (keyword.trim()) params.keyword = keyword.trim();

      const response = await queryRequirements(params);
      setData(response.data || []);
      setTotal(response.total || 0);
    } catch (err: unknown) {
      const e = err as { response?: { data?: { message?: string } }; message?: string };
      setError(e.response?.data?.message || e.message || '查询失败，请检查网络连接');
      setData([]);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchData();
  }, [page, searchTrigger]);

  const handleSearch = (e: React.FormEvent) => {
    e.preventDefault();
    setPage(1);
    setSearchTrigger((t) => t + 1);
  };

  const toggleExpand = (id: number) => {
    setExpandedId(expandedId === id ? null : id);
  };

  const contentSummary = (content: string, maxLen: number = 80) => {
    if (!content) return '';
    return content.length <= maxLen ? content : content.slice(0, maxLen) + '...';
  };

  const totalPages = Math.ceil(total / limit) || 1;

  return (
    <div className="requirement-query">
      <h2>需求查询</h2>

      <form onSubmit={handleSearch} className="query-form">
        <div className="form-row">
          <div className="form-group">
            <label htmlFor="keyword">关键词搜索</label>
            <input
              id="keyword"
              type="text"
              value={keyword}
              onChange={(e) => setKeyword(e.target.value)}
              placeholder="搜索题目或内容"
              disabled={loading}
            />
          </div>
          <div className="form-group">
            <label htmlFor="willing_to_pay">愿意付费</label>
            <select
              id="willing_to_pay"
              value={willingToPay}
              onChange={(e) => setWillingToPay(e.target.value === '' ? '' : Number(e.target.value))}
              disabled={loading}
            >
              <option value="">全部</option>
              <option value="1">愿意</option>
              <option value="0">不愿意</option>
              <option value="2">未填</option>
            </select>
          </div>
          <div className="form-group form-group-btn">
            <button type="submit" className="btn-query" disabled={loading}>
              {loading ? '查询中...' : '查询'}
            </button>
          </div>
        </div>
      </form>

      {error && <div className="error-message">{error}</div>}

      <div className="query-results">
        <div className="results-header">
          <h3>最近 100 条需求（共 {total} 条）</h3>
        </div>

        {loading && data.length === 0 ? (
          <div className="loading">加载中...</div>
        ) : data.length === 0 ? (
          <div className="no-data">暂无数据</div>
        ) : (
          <div className="requirement-list">
            {data.map((item) => (
              <div key={item.id} className="requirement-item">
                <div
                  className="requirement-header"
                  onClick={() => toggleExpand(item.id)}
                  role="button"
                  tabIndex={0}
                  onKeyDown={(e) => e.key === 'Enter' && toggleExpand(item.id)}
                >
                  <div className="requirement-title">{item.title}</div>
                  <div className="requirement-meta">
                    <span className={`willing-badge willing-badge-${item.willing_to_pay ?? 'null'}`}>
                      {getWillingToPayLabel(item.willing_to_pay)}
                    </span>
                    <span className="created-at">{item.created_at}</span>
                  </div>
                  <span className="expand-icon">{expandedId === item.id ? '▼' : '▶'}</span>
                </div>
                <div className="requirement-summary">{contentSummary(item.content)}</div>
                {expandedId === item.id && (
                  <div className="requirement-detail">
                    <div className="detail-section">
                      <strong>内容：</strong>
                      <pre>{item.content}</pre>
                    </div>
                    {item.contact && (
                      <div className="detail-section">
                        <strong>联系方式：</strong>
                        <span>{item.contact}</span>
                      </div>
                    )}
                    {item.notes && (
                      <div className="detail-section">
                        <strong>备注：</strong>
                        <pre>{item.notes}</pre>
                      </div>
                    )}
                  </div>
                )}
              </div>
            ))}
          </div>
        )}

        {totalPages > 1 && (
          <div className="pagination">
            <button
              type="button"
              onClick={() => setPage((p) => Math.max(1, p - 1))}
              disabled={page <= 1 || loading}
            >
              上一页
            </button>
            <span>
              第 {page} / {totalPages} 页
            </span>
            <button
              type="button"
              onClick={() => setPage((p) => Math.min(totalPages, p + 1))}
              disabled={page >= totalPages || loading}
            >
              下一页
            </button>
          </div>
        )}
      </div>
    </div>
  );
}
