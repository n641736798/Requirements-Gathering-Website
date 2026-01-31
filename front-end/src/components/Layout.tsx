import { Link, Outlet, useLocation } from 'react-router-dom';
import './Layout.css';

export default function Layout() {
  const location = useLocation();

  return (
    <div className="layout">
      <header className="header">
        <div className="header-content">
          <h1 className="logo">需求收集网站</h1>
          <nav className="nav">
            <Link
              to="/report"
              className={location.pathname === '/report' ? 'nav-link active' : 'nav-link'}
            >
              需求上报
            </Link>
            <Link
              to="/query"
              className={location.pathname === '/query' ? 'nav-link active' : 'nav-link'}
            >
              需求查询
            </Link>
          </nav>
        </div>
      </header>

      <main className="main-content">
        <Outlet />
      </main>

      <footer className="footer">
        <p>&copy; 2026 需求收集网站</p>
      </footer>
    </div>
  );
}
