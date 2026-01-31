import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import Layout from './components/Layout';
import DeviceReport from './components/DeviceReport';
import DeviceQuery from './components/DeviceQuery';
import './App.css';

function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<Layout />}>
          <Route index element={<Navigate to="/report" replace />} />
          <Route path="report" element={<DeviceReport />} />
          <Route path="query" element={<DeviceQuery />} />
        </Route>
      </Routes>
    </BrowserRouter>
  );
}

export default App;
