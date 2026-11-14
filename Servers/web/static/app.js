/* ==========================================================================
   PlusOS 2.0 Web Client Scripts (plusos.d0a.net)
   Created for the1Domo
   ========================================================================== */

document.addEventListener('DOMContentLoaded', () => {
  // 1. Matrix Digital Rain Canvas Overlay
  const canvas = document.getElementById('matrix-canvas');
  let matrixRunning = true;
  let animationFrameId = null;

  if (canvas) {
    const ctx = canvas.getContext('2d');
    let width = canvas.width = window.innerWidth;
    let height = canvas.height = window.innerHeight;

    const chars = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZﾊﾐﾋｰｳｼﾅﾓﾆｻﾜﾂｵﾘｱﾎﾃﾏｹﾒｴｶｷﾑﾕﾗｾﾈｽﾀﾇﾍ+-*/=%$#@&<>';
    const fontSize = 14;
    let columns = Math.floor(width / fontSize);
    let drops = [];

    const initDrops = () => {
      columns = Math.floor(width / fontSize);
      drops = [];
      for (let i = 0; i < columns; i++) {
        drops[i] = Math.floor(Math.random() * -50);
      }
    };
    initDrops();

    window.addEventListener('resize', () => {
      width = canvas.width = window.innerWidth;
      height = canvas.height = window.innerHeight;
      initDrops();
    });

    let lastDraw = 0;
    const drawMatrix = (timestamp) => {
      if (!matrixRunning) return;
      if (timestamp - lastDraw > 45) { // ~22 FPS for authentic vintage CRT feel
        lastDraw = timestamp;

        ctx.fillStyle = 'rgba(5, 13, 8, 0.08)';
        ctx.fillRect(0, 0, width, height);

        ctx.fillStyle = '#00ff66';
        ctx.font = `${fontSize}px monospace`;

        for (let i = 0; i < drops.length; i++) {
          const char = chars[Math.floor(Math.random() * chars.length)];
          const x = i * fontSize;
          const y = drops[i] * fontSize;

          // Leading bright glyph
          ctx.fillStyle = '#c8ffdb';
          ctx.fillText(char, x, y);
          ctx.fillStyle = '#00ff66';

          if (y > height && Math.random() > 0.975) {
            drops[i] = 0;
          }
          drops[i]++;
        }
      }
      animationFrameId = requestAnimationFrame(drawMatrix);
    };

    animationFrameId = requestAnimationFrame(drawMatrix);

    // Matrix Rain Toggle Button
    const matrixToggle = document.getElementById('matrix-toggle');
    if (matrixToggle) {
      matrixToggle.addEventListener('click', (e) => {
        e.preventDefault();
        matrixRunning = !matrixRunning;
        if (matrixRunning) {
          canvas.style.display = 'block';
          matrixToggle.textContent = 'Matrix Rain: ON';
          animationFrameId = requestAnimationFrame(drawMatrix);
        } else {
          canvas.style.display = 'none';
          matrixToggle.textContent = 'Matrix Rain: OFF';
          if (animationFrameId) cancelAnimationFrame(animationFrameId);
        }
      });
    }
  }

  // 2. CRT Glow Theme Switcher
  const themeBtn = document.getElementById('theme-toggle');
  if (themeBtn) {
    const savedTheme = localStorage.getItem('plusos_theme');
    if (savedTheme === 'crt') {
      document.body.setAttribute('data-theme', 'crt');
      themeBtn.textContent = 'CRT Mode: ON';
    }
    themeBtn.addEventListener('click', (e) => {
      e.preventDefault();
      const current = document.body.getAttribute('data-theme');
      if (current === 'crt') {
        document.body.removeAttribute('data-theme');
        localStorage.setItem('plusos_theme', 'standard');
        themeBtn.textContent = 'CRT Mode: OFF';
      } else {
        document.body.setAttribute('data-theme', 'crt');
        localStorage.setItem('plusos_theme', 'crt');
        themeBtn.textContent = 'CRT Mode: ON';
      }
    });
  }

  // 3. Real-time Clock in Menubar
  const clockEl = document.getElementById('menubar-clock');
  if (clockEl) {
    const updateClock = () => {
      const now = new Date();
      let hours = now.getHours();
      const minutes = String(now.getMinutes()).padStart(2, '0');
      const ampm = hours >= 12 ? 'PM' : 'AM';
      hours = hours % 12 || 12;
      clockEl.textContent = `${hours}:${minutes} ${ampm}`;
    };
    updateClock();
    setInterval(updateClock, 30000);
  }

  // 4. Interactive Showcase Filter Tabs
  const filterButtons = document.querySelectorAll('.showcase-filter-btn');
  const showcaseItems = document.querySelectorAll('.showcase-item');

  if (filterButtons.length) {
    filterButtons.forEach(btn => {
      btn.addEventListener('click', () => {
        filterButtons.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');

        const cat = btn.getAttribute('data-category');
        showcaseItems.forEach(item => {
          if (cat === 'all' || item.getAttribute('data-category') === cat) {
            item.style.display = 'block';
          } else {
            item.style.display = 'none';
          }
        });
      });
    });
  }

  // 5. Image Lightbox Modal
  const modal = document.getElementById('image-modal');
  const modalImg = document.getElementById('modal-img');
  const modalCaption = document.getElementById('modal-caption');
  const modalClose = document.getElementById('modal-close');

  if (modal && modalImg) {
    document.querySelectorAll('.zoomable-img').forEach(el => {
      el.addEventListener('click', () => {
        const src = el.getAttribute('data-src') || el.getAttribute('src');
        const caption = el.getAttribute('data-caption') || el.getAttribute('alt') || '';
        modalImg.src = src;
        if (modalCaption) modalCaption.textContent = caption;
        modal.classList.add('open');
      });
    });

    if (modalClose) {
      modalClose.addEventListener('click', () => modal.classList.remove('open'));
    }
    modal.addEventListener('click', (e) => {
      if (e.target === modal) modal.classList.remove('open');
    });
    document.addEventListener('keydown', (e) => {
      if (e.key === 'Escape' && modal.classList.contains('open')) {
        modal.classList.remove('open');
      }
    });
  }

  // 6. Wiki Search Filter
  const searchInput = document.getElementById('wiki-search-input');
  if (searchInput) {
    searchInput.addEventListener('input', (e) => {
      const term = e.target.value.toLowerCase().trim();
      const items = document.querySelectorAll('.wiki-nav-list li');
      items.forEach(li => {
        const text = li.textContent.toLowerCase();
        if (text.includes(term)) {
          li.style.display = '';
        } else {
          li.style.display = 'none';
        }
      });
    });
  }

  // 7. Wiki Editor Form submission
  const editorForm = document.getElementById('wiki-editor-form');
  if (editorForm) {
    editorForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const statusEl = document.getElementById('editor-status');
      const submitBtn = document.getElementById('editor-submit-btn');

      const slug = document.getElementById('edit-slug').value.trim();
      const title = document.getElementById('edit-title').value.trim();
      const category = document.getElementById('edit-category').value.trim();
      const summary = document.getElementById('edit-summary').value.trim();
      const content = document.getElementById('edit-content').value;
      const adminKey = document.getElementById('edit-key').value;

      if (!slug || !title || !content) {
        alert('Slug, Title, and Content are required!');
        return;
      }

      submitBtn.disabled = true;
      if (statusEl) statusEl.textContent = 'Saving article to server...';

      try {
        const res = await fetch('/api/wiki/save', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ slug, title, category, summary, content, admin_key: adminKey })
        });
        const data = await res.json();
        if (data.ok) {
          if (statusEl) statusEl.textContent = 'Saved successfully! Redirecting...';
          window.location.href = `/wiki/${encodeURIComponent(slug)}`;
        } else {
          alert('Error saving article: ' + (data.error || 'Unknown error'));
          if (statusEl) statusEl.textContent = 'Save failed.';
          submitBtn.disabled = false;
        }
      } catch (err) {
        alert('Server connection error: ' + err.message);
        if (statusEl) statusEl.textContent = 'Connection error.';
        submitBtn.disabled = false;
      }
    });
  }
});
