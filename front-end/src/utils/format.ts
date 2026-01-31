// 格式化时间戳为可读时间
export const formatTimestamp = (timestamp: number): string => {
  const date = new Date(timestamp * 1000); // 假设是秒级时间戳
  return date.toLocaleString('zh-CN', {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  });
};

// 获取当前时间戳（秒）
export const getCurrentTimestamp = (): number => {
  return Math.floor(Date.now() / 1000);
};
