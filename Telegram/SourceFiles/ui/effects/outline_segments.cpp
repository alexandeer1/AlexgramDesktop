/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/effects/outline_segments.h"

#include "ui/painter.h"

namespace Ui {

constexpr auto kPi = 3.14159265358979323846;

void PaintOutlineSegments(
		QPainter &p,
		QRectF ellipse,
		const std::vector<OutlineSegment> &segments,
		float64 fromFullProgress) {
	Expects(!segments.empty());

	p.setBrush(Qt::NoBrush);
	const auto count = std::min(int(segments.size()), kOutlineSegmentsMax);
	if (count == 1) {
		p.setPen(QPen(segments.front().brush, segments.front().width));
		p.drawEllipse(ellipse);
		return;
	}
	const auto small = 160;
	const auto full = arc::kFullLength;
	const auto separator = (full > 1.1 * small * count)
		? small
		: (full / (count * 1.1));
	const auto left = full - (separator * count);
	const auto length = left / float64(count);
	const auto spin = separator * (1. - fromFullProgress);

	auto start = 0. + (arc::kQuarterLength + (separator / 2)) + (3. * spin);
	auto pen = QPen(
		segments.back().brush,
		segments.back().width,
		Qt::SolidLine,
		Qt::RoundCap);
	p.setPen(pen);
	for (auto i = 0; i != count;) {
		const auto &segment = segments[count - (++i)];
		if (!segment.width) {
			start += length + separator;
			continue;
		} else if (pen.brush() != segment.brush
			|| pen.widthF() != segment.width) {
			pen = QPen(
				segment.brush,
				segment.width,
				Qt::SolidLine,
				Qt::RoundCap);
			p.setPen(pen);
		}
		const auto from = int(base::SafeRound(start));
		const auto till = start + length;
		auto added = spin;
		for (; i != count;) {
			start += length + separator;
			const auto &next = segments[count - (++i)];
			if (next.width) {
				--i;
				break;
			}
			added += (separator + length) * (1. - fromFullProgress);
		}
		p.drawArc(ellipse, from, int(base::SafeRound(till + added)) - from);
	}
}

void PaintOutlineSegments(
		QPainter &p,
		QRectF rect,
		float64 radius,
		const std::vector<OutlineSegment> &segments) {
	Expects(!segments.empty());

	p.setBrush(Qt::NoBrush);
	const auto count = std::min(int(segments.size()), kOutlineSegmentsMax);
	if (count == 1) {
		p.setPen(QPen(segments.back().brush, segments.back().width));
		if (radius <= 0.) {
			p.drawRect(rect);
		} else {
			p.drawRoundedRect(rect, radius, radius);
		}
		return;
	}

	const auto straightW = rect.width() - 2. * radius;
	const auto straightH = rect.height() - 2. * radius;
	const auto cornerArc = kPi * radius;
	const auto perimeter = 2. * (straightW + straightH) + 2. * cornerArc;

	const auto gapFraction = 0.03;
	const auto totalGap = gapFraction * perimeter * count;
	const auto segmentTotal = perimeter - totalGap;
	const auto segLen = segmentTotal / count;
	const auto gapLen = totalGap / count;

	QPainterPath path;
	path.addRoundedRect(rect, radius, radius);

	auto hq = PainterHighQualityEnabler(p);
	for (auto i = 0; i != count; ++i) {
		const auto &seg = segments[count - 1 - i];
		if (!seg.width) {
			continue;
		}
		const auto offset = i * (segLen + gapLen);
		auto pen = QPen(seg.brush, seg.width, Qt::SolidLine, Qt::RoundCap);
		pen.setDashPattern({
			segLen / seg.width,
			(perimeter - segLen) / seg.width,
		});
		pen.setDashOffset(offset / seg.width);
		p.setPen(pen);
		p.drawPath(path);
	}
}


QLinearGradient UnreadStoryOutlineGradient(
		QRectF rect,
		const QColor &c1,
		const QColor &c2) {
	auto result = QLinearGradient(rect.topRight(), rect.bottomLeft());
	result.setStops({ { 0., c1 }, { 1., c2 } });
	return result;
}

QLinearGradient UnreadStoryOutlineGradient(QRectF rect) {
	return UnreadStoryOutlineGradient(
		std::move(rect),
		st::groupCallLive1->c,
		st::groupCallMuted1->c);
}

} // namespace Ui
