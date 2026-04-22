#include "CloudSeamanor/SpriteAnimator.hpp"
#include <algorithm>
namespace CloudSeamanor::engine::animation {
void SpriteAnimator::LoadSpritesheet(const sf::Texture& t, int fw, int fh) { spritesheet_=&t; frame_width_=fw; frame_height_=fh; }
void SpriteAnimator::AddAnimation(const std::string& name, const Animation& anim) { animations_[name]=anim; }
void SpriteAnimator::AddAnimation(const std::string& name, int col, int row, int cnt, float dur, bool loop) {
    Animation a; a.name=name; a.loop=loop;
    for (int i=0;i<cnt;++i) a.frames.push_back({sf::IntRect{col*frame_width_+i*frame_width_,row*frame_height_,frame_width_,frame_height_},dur});
    animations_[name]=std::move(a);
}
void SpriteAnimator::Play(const std::string& name) {
    auto it=animations_.find(name); if(it==animations_.end()) return;
    current_name_=name; current_anim_=&it->second; current_frame_index_=0; frame_timer_=0.0f; playing_=true; paused_=false; finished_=false;
}
void SpriteAnimator::Resume(const std::string& name) {
    auto it=animations_.find(name); if(it==animations_.end()) return;
    current_name_=name; current_anim_=&it->second; playing_=true; paused_=false;
}
void SpriteAnimator::Stop() { playing_=false; }
void SpriteAnimator::Pause() { paused_=true; }
void SpriteAnimator::SetSpeed(float m) { speed_multiplier_=m; }
void SpriteAnimator::Update(float dt) {
    if(!playing_||!current_anim_||paused_) return;
    frame_timer_+=dt*speed_multiplier_;
    float fd=current_anim_->frames[current_frame_index_].duration_seconds;
    if(frame_timer_>=fd) {
        frame_timer_-=fd; ++current_frame_index_;
        if(current_frame_index_>=current_anim_->frames.size()) {
            if(current_anim_->loop) current_frame_index_=0;
            else { current_frame_index_=current_anim_->frames.size()-1; playing_=false; finished_=true; }
        }
    }
}
const sf::IntRect& SpriteAnimator::CurrentRect() const { static sf::IntRect e; return (!current_anim_||current_frame_index_>=current_anim_->frames.size())?e:current_anim_->frames[current_frame_index_].rect; }
std::size_t SpriteAnimator::TotalFrames() const { return current_anim_?current_anim_->frames.size():0; }
void SpriteAnimator::ApplyTo(sf::Sprite& s) const { s.setTextureRect(CurrentRect()); }
SpriteAnimator Create8DirectionAnimator(const sf::Texture& t, int fw, int fh) {
    SpriteAnimator a; a.LoadSpritesheet(t,fw,fh);
    const char* dirs[]={down,left,right,up,down_left,down_right,up_left,up_right};
    for(int d=0;d<8;++d) a.AddAnimation(dirs[d],0,d,4,0.125f,true);
    return a;
}
SpriteAnimator Create4FrameAnimator(const sf::Texture& t, int row, int fw, int fh) {
    SpriteAnimator a; a.LoadSpritesheet(t,fw,fh); a.AddAnimation(walk,0,row,4,0.125f,true); return a;
}
}  // namespace CloudSeamanor::engine::animation