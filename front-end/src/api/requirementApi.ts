import axios from 'axios';

const API_BASE_URL = '/api/v1';

export interface RequirementReport {
  title: string;
  content: string;
  willing_to_pay?: number | null;  // 0=不愿意, 1=愿意, null=空
  contact?: string;
  notes?: string;
}

export interface RequirementReportResponse {
  code: number;
  message: string;
}

export interface Requirement {
  id: number;
  title: string;
  content: string;
  willing_to_pay: number | null;
  contact: string;
  notes: string;
  created_at: string;
  updated_at: string;
}

export interface RequirementQueryResponse {
  code: number;
  data: Requirement[];
  total: number;
  page: number;
  limit: number;
}

// 请求超时时间（毫秒），避免请求挂起导致一直显示"提交中"
const REQUEST_TIMEOUT = 15000;

// 需求上报
export const reportRequirement = async (
  data: RequirementReport
): Promise<RequirementReportResponse> => {
  const response = await axios.post<RequirementReportResponse>(
    `${API_BASE_URL}/requirement/report`,
    data,
    {
      headers: {
        'Content-Type': 'application/json; charset=utf-8',
      },
      timeout: REQUEST_TIMEOUT,
    }
  );
  return response.data;
};

// 需求查询
export const queryRequirements = async (params?: {
  page?: number;
  limit?: number;
  willing_to_pay?: number;
  keyword?: string;
}): Promise<RequirementQueryResponse> => {
  const response = await axios.get<RequirementQueryResponse>(
    `${API_BASE_URL}/requirement/query`,
    { params: params || {}, timeout: REQUEST_TIMEOUT }
  );
  return response.data;
};
