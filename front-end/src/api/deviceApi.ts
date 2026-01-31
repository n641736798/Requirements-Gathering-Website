import axios from 'axios';

const API_BASE_URL = '/api/v1';

export interface DeviceReport {
  device_id: string;
  timestamp: number;
  metrics: Record<string, number | string>;
}

export interface DeviceReportResponse {
  code: number;
  message: string;
}

export interface DeviceDataPoint {
  timestamp: number;
  [key: string]: number | string;
}

export interface DeviceQueryResponse {
  device_id: string;
  data: DeviceDataPoint[];
}

// 设备数据上报
export const reportDeviceData = async (data: DeviceReport): Promise<DeviceReportResponse> => {
  const response = await axios.post<DeviceReportResponse>(
    `${API_BASE_URL}/device/report`,
    data,
    {
      headers: {
        'Content-Type': 'application/json; charset=utf-8',
      },
    }
  );
  return response.data;
};

// 设备数据查询
export const queryDeviceData = async (
  deviceId: string,
  limit: number = 100
): Promise<DeviceQueryResponse> => {
  const response = await axios.get<DeviceQueryResponse>(
    `${API_BASE_URL}/device/query`,
    {
      params: {
        device_id: deviceId,
        limit: limit,
      },
    }
  );
  return response.data;
};
