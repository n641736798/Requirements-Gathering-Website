import { useState } from 'react';
import { reportRequirement, RequirementReport as RequirementReportData } from '../api/requirementApi';
import './RequirementReport.css';

export default function RequirementReport() {
  const [title, setTitle] = useState('');
  const [content, setContent] = useState('');
  const [willingToPay, setWillingToPay] = useState<number | null | ''>(null);
  const [contact, setContact] = useState('');
  const [notes, setNotes] = useState('');
  const [loading, setLoading] = useState(false);
  const [message, setMessage] = useState<{ type: 'success' | 'error'; text: string } | null>(null);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setLoading(true);
    setMessage(null);

    // 安全超时：20 秒后强制重置，防止请求挂起导致一直显示"提交中"
    const safetyTimer = setTimeout(() => {
      setLoading((prev) => {
        if (prev) {
          setMessage({ type: 'error', text: '请求超时，请检查后端服务与 Nginx 配置' });
          return false;
        }
        return prev;
      });
    }, 20000);

    try {
      if (!title.trim()) {
        throw new Error('题目不能为空');
      }
      if (!content.trim()) {
        throw new Error('内容不能为空');
      }

      const reportData: RequirementReportData = {
        title: title.trim(),
        content: content.trim(),
        willing_to_pay: willingToPay === '' ? null : willingToPay,
        contact: contact.trim() || undefined,
        notes: notes.trim() || undefined,
      };

      const response = await reportRequirement(reportData);

      if (response?.code === 0) {
        setMessage({ type: 'success', text: '需求上报成功！' });
        setTitle('');
        setContent('');
        setWillingToPay(null);
        setContact('');
        setNotes('');;
      } else {
        throw new Error(response?.message || '上报失败');
      }
    } catch (error: unknown) {
      const err = error as { response?: { status?: number; data?: { message?: string } }; message?: string; code?: string };
      let text = err?.response?.data?.message || err?.message || '上报失败，请检查网络连接';
      const msg = String(err?.message || '');
      if (err?.response?.status === 504) {
        text = '504 网关超时：请检查 Nginx proxy_read_timeout 及后端服务';
      } else if (err?.code === 'ECONNABORTED' || msg.includes('timeout')) {
        text = '请求超时，请确认后端服务已启动（端口 8080）';
      } else if (msg.includes('Network Error') || err?.code === 'ERR_NETWORK') {
        text = '无法连接服务器，请确认后端服务已启动（端口 8080）';
      }
      setMessage({ type: 'error', text });
    } finally {
      clearTimeout(safetyTimer);
      setLoading(false);
    }
  };

  return (
    <div className="requirement-report">
      <h2>需求上报</h2>
      <form onSubmit={handleSubmit} className="report-form">
        <div className="form-group">
          <label htmlFor="title">题目 *</label>
          <input
            id="title"
            type="text"
            value={title}
            onChange={(e) => setTitle(e.target.value)}
            placeholder="请输入需求题目"
            required
            disabled={loading}
          />
        </div>

        <div className="form-group">
          <label htmlFor="content">内容 *</label>
          <textarea
            id="content"
            value={content}
            onChange={(e) => setContent(e.target.value)}
            placeholder="请详细描述您的需求"
            required
            disabled={loading}
            rows={6}
          />
        </div>

        <div className="form-group">
          <label htmlFor="willing_to_pay">愿意付费</label>
          <select
            id="willing_to_pay"
            value={willingToPay === null ? '' : willingToPay}
            onChange={(e) => {
              const v = e.target.value;
              setWillingToPay(v === '' ? null : Number(v));
            }}
            disabled={loading}
          >
            <option value="">不填</option>
            <option value="1">愿意</option>
            <option value="0">不愿意</option>
          </select>
        </div>

        <div className="form-group">
          <label htmlFor="contact">联系方式（选填）</label>
          <input
            id="contact"
            type="text"
            value={contact}
            onChange={(e) => setContact(e.target.value)}
            placeholder="邮箱、电话等"
            disabled={loading}
          />
        </div>

        <div className="form-group">
          <label htmlFor="notes">备注（选填）</label>
          <textarea
            id="notes"
            value={notes}
            onChange={(e) => setNotes(e.target.value)}
            placeholder="其他补充说明"
            disabled={loading}
            rows={3}
          />
        </div>

        {message && (
          <div className={`message message-${message.type}`}>{message.text}</div>
        )}

        <button type="submit" className="btn-submit" disabled={loading}>
          {loading ? '提交中...' : '提交'}
        </button>
      </form>
    </div>
  );
}
