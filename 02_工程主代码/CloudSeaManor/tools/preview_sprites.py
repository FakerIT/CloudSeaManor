#!/usr/bin/env python3
"""
【Sprite Preview Generator】美术资源预览生成器

用途：根据 sprite_mapping.csv 生成 HTML 预览页面，方便策划/美术人员
      查看所有资源 ID 对应的实际图片。

使用方法：
    python preview_sprites.py [--csv assets/configs/sprite_mapping.csv]

输出：
    - preview/sprite_preview.html (预览页面)

依赖：
    pip install Pillow  # 图片缩略图生成
"""

import csv
import os
import sys
import argparse
from pathlib import Path
from typing import Optional, Dict, List, Tuple
from dataclasses import dataclass
from html import escape

# 可选依赖
try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


@dataclass
class SpriteInfo:
    sprite_id: str
    category: str
    file_path: str
    frame_count: int
    fps: int
    width: int
    height: int
    description: str
    atlas_source: str


class SpritePreviewGenerator:
    def __init__(self, base_path: str = ""):
        self.base_path = Path(base_path) if base_path else Path.cwd()
        self.sprites: List[SpriteInfo] = []
        self.categories: Dict[str, int] = {}
        self.missing_files: List[str] = []
        self.existing_files: List[str] = []

    def load_csv(self, csv_path: str) -> bool:
        """加载 CSV 映射表"""
        full_path = self.base_path / csv_path if not os.path.isabs(csv_path) else Path(csv_path)

        if not full_path.exists():
            print(f"错误: CSV 文件不存在: {full_path}")
            return False

        try:
            with open(full_path, 'r', encoding='utf-8') as f:
                reader = csv.DictReader(f)
                for row in reader:
                    # 跳过注释行和空行
                    if not row.get('SpriteId') or row.get('SpriteId', '').strip().startswith('#'):
                        continue

                    sprite = SpriteInfo(
                        sprite_id=row.get('SpriteId', '').strip(),
                        category=row.get('Category', 'Uncategorized').strip(),
                        file_path=row.get('FilePath', '').strip(),
                        frame_count=int(row.get('FrameCount', 1) or 1),
                        fps=int(row.get('FPS', 0) or 0),
                        width=int(row.get('Width', 0) or 0),
                        height=int(row.get('Height', 0) or 0),
                        description=row.get('Description', '').strip(),
                        atlas_source=row.get('AtlasSource', '').strip()
                    )
                    self.sprites.append(sprite)

                    # 统计分类
                    self.categories[sprite.category] = self.categories.get(sprite.category, 0) + 1

            print(f"成功加载 {len(self.sprites)} 个资源")
            return True

        except Exception as e:
            print(f"错误: 读取 CSV 失败: {e}")
            return False

    def validate_files(self, sprites_root: str = "assets/sprites") -> Tuple[int, int]:
        """验证文件是否存在"""
        sprites_path = self.base_path / sprites_root

        for sprite in self.sprites:
            full_path = sprites_path / sprite.file_path
            if full_path.exists():
                self.existing_files.append(str(full_path))
            else:
                self.missing_files.append(str(full_path))

        return len(self.existing_files), len(self.missing_files)

    def generate_thumbnail(self, source_path: str, thumb_size: Tuple[int, int] = (64, 64)) -> Optional[str]:
        """生成缩略图（Base64 编码）"""
        if not HAS_PIL:
            return None

        try:
            img = Image.open(source_path)
            img.thumbnail(thumb_size, Image.Resampling.LANCZOS)

            import io
            buffer = io.BytesIO()
            img_format = 'PNG' if img.mode == 'RGBA' else 'JPEG'
            img.save(buffer, format=img_format)
            import base64
            return base64.b64encode(buffer.getvalue()).decode('utf-8')
        except Exception as e:
            return None

    def generate_html(self, output_path: str = "preview/sprite_preview.html",
                      sprites_root: str = "assets/sprites") -> bool:
        """生成 HTML 预览页面"""
        sprites_path = self.base_path / sprites_root

        # 统计
        existing_count, missing_count = self.validate_files(sprites_root)

        # 按分类分组
        categories_sorted = sorted(self.categories.items(), key=lambda x: -x[1])

        html = self._generate_html_header()
        html += self._generate_stats_section(existing_count, missing_count)
        html += self._generate_category_nav(categories_sorted)

        # 生成每个分类的内容
        current_category = None
        for sprite in sorted(self.sprites, key=lambda x: (x.category, x.sprite_id)):
            if sprite.category != current_category:
                if current_category is not None:
                    html += "</div>\n"
                html += f'<div id="cat-{escape(sprite.category)}" class="category-section">\n'
                html += f'<h2 class="category-title">{escape(sprite.category)} <span class="count">({self.categories[sprite.category]})</span></h2>\n'
                current_category = sprite.category

            html += self._generate_sprite_card(sprite, sprites_path)

        if current_category is not None:
            html += "</div>\n"

        html += self._generate_html_footer()

        # 写入文件
        output_file = self.base_path / output_path
        output_file.parent.mkdir(parents=True, exist_ok=True)

        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(html)

        print(f"预览页面已生成: {output_file}")
        return True

    def _generate_html_header(self) -> str:
        return '''<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>云海山庄 - 美术资源预览</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Microsoft YaHei", sans-serif;
            background: #1a1a2e;
            color: #eee;
            line-height: 1.6;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 2rem;
            text-align: center;
        }
        .header h1 { font-size: 2rem; margin-bottom: 0.5rem; }
        .header p { opacity: 0.9; }
        .stats {
            display: flex;
            justify-content: center;
            gap: 2rem;
            padding: 1.5rem;
            background: #16213e;
        }
        .stat {
            text-align: center;
        }
        .stat-value {
            font-size: 2rem;
            font-weight: bold;
        }
        .stat-label { opacity: 0.7; font-size: 0.9rem; }
        .stat.ok .stat-value { color: #4ade80; }
        .stat.missing .stat-value { color: #f87171; }
        .nav {
            position: sticky;
            top: 0;
            background: #16213e;
            padding: 1rem;
            display: flex;
            flex-wrap: wrap;
            gap: 0.5rem;
            z-index: 100;
            border-bottom: 1px solid #333;
        }
        .nav a {
            color: #94a3b8;
            text-decoration: none;
            padding: 0.5rem 1rem;
            border-radius: 4px;
            transition: all 0.2s;
        }
        .nav a:hover {
            background: #334155;
            color: #fff;
        }
        .nav a.active {
            background: #667eea;
            color: #fff;
        }
        .container { max-width: 1400px; margin: 0 auto; padding: 1rem; }
        .category-section {
            margin: 2rem 0;
        }
        .category-title {
            font-size: 1.3rem;
            margin-bottom: 1rem;
            padding-bottom: 0.5rem;
            border-bottom: 2px solid #334155;
        }
        .category-title .count { font-size: 0.9rem; opacity: 0.7; }
        .sprite-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
            gap: 1rem;
        }
        .sprite-card {
            background: #16213e;
            border-radius: 8px;
            overflow: hidden;
            transition: transform 0.2s, box-shadow 0.2s;
        }
        .sprite-card:hover {
            transform: translateY(-4px);
            box-shadow: 0 8px 25px rgba(0,0,0,0.3);
        }
        .sprite-preview {
            height: 100px;
            display: flex;
            align-items: center;
            justify-content: center;
            background: #0f172a;
            position: relative;
        }
        .sprite-preview img {
            max-width: 100%;
            max-height: 100%;
            object-fit: contain;
            image-rendering: pixelated;
        }
        .sprite-preview.missing {
            color: #f87171;
            font-size: 0.8rem;
        }
        .sprite-preview.atlas {
            background: repeating-conic-gradient(#1e293b 0% 25%, #0f172a 0% 50%) 50% / 10px 10px;
        }
        .sprite-info {
            padding: 0.75rem;
        }
        .sprite-id {
            font-family: monospace;
            font-size: 0.85rem;
            color: #67e8f9;
            word-break: break-all;
        }
        .sprite-desc {
            font-size: 0.8rem;
            opacity: 0.7;
            margin-top: 0.25rem;
        }
        .sprite-meta {
            font-size: 0.7rem;
            opacity: 0.5;
            margin-top: 0.25rem;
        }
        .missing-list {
            margin: 2rem;
            background: #7f1d1d;
            border-radius: 8px;
            padding: 1rem;
        }
        .missing-list h3 { color: #fca5a5; margin-bottom: 0.5rem; }
        .missing-list ul { list-style: none; }
        .missing-list li {
            font-family: monospace;
            font-size: 0.8rem;
            padding: 0.25rem 0;
            color: #fca5a5;
        }
        .footer {
            text-align: center;
            padding: 2rem;
            opacity: 0.5;
            font-size: 0.8rem;
        }
        .atlas-badge {
            position: absolute;
            top: 4px;
            right: 4px;
            background: #6366f1;
            color: white;
            font-size: 0.6rem;
            padding: 2px 6px;
            border-radius: 4px;
        }
    </style>
</head>
<body>
    <div class="header">
        <h1>云海山庄 - 美术资源预览</h1>
        <p>通过 sprite_mapping.csv 自动生成 | 策划换皮参考</p>
    </div>
    <div class="stats">
'''

    def _generate_stats_section(self, existing: int, missing: int) -> str:
        total = existing + missing
        html = f'''
        <div class="stat ok">
            <div class="stat-value">{existing}</div>
            <div class="stat-label">已存在</div>
        </div>
        <div class="stat{' missing' if missing > 0 else ''}">
            <div class="stat-value">{missing}</div>
            <div class="stat-label">缺失文件</div>
        </div>
        <div class="stat">
            <div class="stat-value">{total}</div>
            <div class="stat-label">总计资源</div>
        </div>
    </div>
'''
        # 添加缺失文件列表
        if missing > 0:
            html += '<div class="missing-list"><h3>⚠️ 缺失文件清单</h3><ul>'
            for f in self.missing_files[:50]:  # 最多显示50个
                html += f'<li>{escape(f)}</li>'
            if len(self.missing_files) > 50:
                html += f'<li>... 还有 {len(self.missing_files) - 50} 个文件</li>'
            html += '</ul></div>'

        return html

    def _generate_category_nav(self, categories: List[Tuple[str, int]]) -> str:
        html = '<div class="nav"><a href="#top">顶部</a>'
        for cat, count in categories:
            html += f'<a href="#cat-{escape(cat)}">{escape(cat)} ({count})</a>'
        html += '</div><div class="container" id="top">'
        return html

    def _generate_sprite_card(self, sprite: SpriteInfo, sprites_path: Path) -> str:
        # 生成预览图片或占位符
        full_path = sprites_path / sprite.file_path
        exists = full_path.exists()

        preview_html = ""
        if exists:
            if HAS_PIL and full_path.suffix.lower() in ['.png', '.jpg', '.jpeg', '.gif', '.bmp']:
                thumbnail = self.generate_thumbnail(str(full_path))
                if thumbnail:
                    import base64
                    img_type = 'png' if full_path.suffix.lower() == '.png' else 'jpeg'
                    preview_html = f'<img src="data:image/{img_type};base64,{thumbnail}" alt="{escape(sprite.sprite_id)}">'
                else:
                    preview_html = f'<img src="../../{sprite.file_path.replace(os.sep, "/")}" alt="{escape(sprite.sprite_id)}" style="max-width:100%;max-height:100%;image-rendering:pixelated;">'
            else:
                preview_html = f'<img src="../../{sprite.file_path.replace(os.sep, "/")}" alt="{escape(sprite.sprite_id)}" style="max-width:100%;max-height:100%;image-rendering:pixelated;">'
        else:
            preview_html = '<span>文件不存在</span>'

        # 元数据
        meta_parts = []
        if sprite.frame_count > 1:
            meta_parts.append(f"{sprite.frame_count}f @{sprite.fps}fps")
        if sprite.width and sprite.height:
            meta_parts.append(f"{sprite.width}×{sprite.height}")
        if sprite.atlas_source:
            meta_parts.append(f"atlas:{sprite.atlas_source}")

        meta_html = '<br>'.join(meta_parts) if meta_parts else ""
        atlas_badge = f'<span class="atlas-badge">{escape(sprite.atlas_source)}</span>' if sprite.atlas_source else ""

        return f'''
        <div class="sprite-card">
            <div class="sprite-preview{' missing' if not exists else ''}{' atlas' if sprite.atlas_source else ''}">
                {atlas_badge}
                {preview_html}
            </div>
            <div class="sprite-info">
                <div class="sprite-id">{escape(sprite.sprite_id)}</div>
                <div class="sprite-desc">{escape(sprite.description)}</div>
                <div class="sprite-meta">{meta_html}</div>
            </div>
        </div>
'''

    def _generate_html_footer(self) -> str:
        return '''
    </div>
    <div class="footer">
        <p>由 preview_sprites.py 自动生成 | 云海山庄</p>
    </div>
    <script>
        // 导航高亮
        const navLinks = document.querySelectorAll('.nav a');
        const sections = document.querySelectorAll('.category-section');

        window.addEventListener('scroll', () => {
            let current = '';
            sections.forEach(section => {
                const sectionTop = section.offsetTop;
                if (scrollY >= sectionTop - 100) {
                    current = section.getAttribute('id');
                }
            });

            navLinks.forEach(link => {
                link.classList.remove('active');
                if (link.getAttribute('href') === '#' + current) {
                    link.classList.add('active');
                }
            });
        });
    </script>
</body>
</html>
'''


def main():
    parser = argparse.ArgumentParser(description='云海山庄 - 美术资源预览生成器')
    parser.add_argument('--csv', '-c', default='assets/configs/sprite_mapping.csv',
                       help='CSV 映射表路径 (默认: assets/configs/sprite_mapping.csv)')
    parser.add_argument('--sprites', '-s', default='assets/sprites',
                       help='资源根目录 (默认: assets/sprites)')
    parser.add_argument('--output', '-o', default='preview/sprite_preview.html',
                       help='输出 HTML 路径 (默认: preview/sprite_preview.html)')
    parser.add_argument('--base', '-b', default='',
                       help='项目根目录 (默认: 当前目录)')
    args = parser.parse_args()

    generator = SpritePreviewGenerator(args.base)

    if not generator.load_csv(args.csv):
        sys.exit(1)

    print("\n文件验证:")
    existing, missing = generator.validate_files(args.sprites)
    print(f"  已存在: {existing}")
    print(f"  缺失文件: {missing}")

    print("\n生成预览页面...")
    if generator.generate_html(args.output, args.sprites):
        print("\n✅ 完成!")
        if missing > 0:
            print(f"⚠️  提示: 有 {missing} 个文件缺失，请检查资源路径或补充图片文件")
    else:
        sys.exit(1)


if __name__ == '__main__':
    main()
