import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import Layout from './components/Layout';
import RequirementReport from './components/RequirementReport';
import RequirementQuery from './components/RequirementQuery';
import './App.css';

function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<Layout />}>
          <Route index element={<Navigate to="/report" replace />} />
          <Route path="report" element={<RequirementReport />} />
          <Route path="query" element={<RequirementQuery />} />
        </Route>
      </Routes>
    </BrowserRouter>
  );
}

export default App;
